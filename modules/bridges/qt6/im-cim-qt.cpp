// -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-
/*
  im-cim-qt.cpp
  This file is part of Cim.

  Copyright (C) 2015-2026 Hodong Kim <hodong@nimfsoft.com>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <clair.h>
#include <atomic>
#ifdef CIM_BRIDGE_TEST
#include <chrono>
#endif
#include <condition_variable>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <utility>
#include <vector>
#include <QByteArray>
#include <QCoreApplication>
#include <QEvent>
#include <QTextFormat>
#include <QInputMethodEvent>
#include <QRect>
#include <QThread>
#include <QtGui/qpa/qplatforminputcontext.h>
#include <QtGui/qpa/qplatforminputcontextplugin_p.h>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include "cim.h"

static bool
utf8_scalar_to_utf16 (const char* text,
                      size_t text_length,
                      uint32_t scalar_offset,
                      int* result)
{
  size_t byte_offset;
  clair_utf8_offset_mapping_t mapping;

  if (result == nullptr)
    return false;

  if (clair_utf8_scalar_offset_to_byte_offset
        (text, text_length, scalar_offset, &byte_offset) != CLAIR_OK ||
      clair_utf8_byte_offset_to_utf16_offset
        (text, text_length, byte_offset, &mapping) != CLAIR_OK ||
      mapping.lower != mapping.upper ||
      mapping.lower > static_cast<size_t>(std::numeric_limits<int>::max()))
    return false;

  *result = static_cast<int>(mapping.lower);
  return true;
}

static bool
utf8_utf16_to_scalar (const char* text,
                      size_t text_length,
                      int utf16_offset,
                      uint32_t* result)
{
  clair_utf8_offset_mapping_t byte_mapping;
  clair_utf8_offset_mapping_t scalar_mapping;

  if (result == nullptr || utf16_offset < 0)
    return false;

  if (clair_utf8_utf16_offset_to_byte_offset
        (text,
         text_length,
         static_cast<size_t>(utf16_offset),
         &byte_mapping) != CLAIR_OK ||
      byte_mapping.lower != byte_mapping.upper ||
      clair_utf8_byte_offset_to_scalar_offset
        (text,
         text_length,
         byte_mapping.lower,
         &scalar_mapping) != CLAIR_OK ||
      scalar_mapping.lower != scalar_mapping.upper ||
      scalar_mapping.lower > std::numeric_limits<uint32_t>::max())
    return false;

  *result = static_cast<uint32_t>(scalar_mapping.lower);
  return true;
}

static bool
utf8_length (const char* text, size_t* result)
{
  size_t length;

  if (text == nullptr || result == nullptr)
    return false;

  length = std::strlen (text);
  if (length > static_cast<size_t>(std::numeric_limits<qsizetype>::max()))
    return false;

  *result = length;
  return true;
}

static bool
qrect_to_cim (const QRect& source, CimRect* result)
{
  if (result == nullptr || source.width () < 0 || source.height () < 0)
    return false;

  int64_t x = source.x ();
  int64_t y = source.y ();
  uint64_t width = static_cast<uint64_t>(source.width ());
  uint64_t height = static_cast<uint64_t>(source.height ());

  if (x < std::numeric_limits<int32_t>::min () ||
      x > std::numeric_limits<int32_t>::max () ||
      y < std::numeric_limits<int32_t>::min () ||
      y > std::numeric_limits<int32_t>::max () ||
      width > std::numeric_limits<uint32_t>::max () ||
      height > std::numeric_limits<uint32_t>::max ())
    return false;

  result->x = static_cast<int32_t>(x);
  result->y = static_cast<int32_t>(y);
  result->width = static_cast<uint32_t>(width);
  result->height = static_cast<uint32_t>(height);

  return true;
}

enum class CimCallbackEventKind
{
  Preedit_Changed,
  Commit
};

static QEvent::Type
cim_callback_event_type ()
{
  static const int type = QEvent::registerEventType ();

  if (type == -1)
    qFatal ("Cim Qt bridge could not allocate a callback event type");

  return static_cast<QEvent::Type>(type);
}

class CimCallbackEvent final : public QEvent
{
public:
  explicit CimCallbackEvent (CimCallbackEventKind event_kind)
    : QEvent (cim_callback_event_type ()),
      kind (event_kind),
      cursor_pos (0)
  {
  }

  CimCallbackEventKind kind;
  std::string text;
  std::vector<CimTextAttr> attrs;
  uint32_t cursor_pos;
};

enum class CimSyncCallbackKind
{
  Get_Surround,
  Delete_Surround
};

struct CimSyncCallbackRequest
{
  explicit CimSyncCallbackRequest (CimSyncCallbackKind request_kind)
    : kind (request_kind)
  {
  }

  CimSyncCallbackKind kind;
  std::mutex mutex;
  std::condition_variable condition;
  bool completed = false;
  bool result = false;
  int32_t offset = 0;
  uint32_t n_chars = 0;
#ifdef CIM_BRIDGE_TEST
  bool test_ready = false;
#endif
};

static QEvent::Type
cim_sync_callback_event_type ()
{
  static const int type = QEvent::registerEventType ();

  if (type == -1)
    qFatal ("Cim Qt bridge could not allocate a synchronous event type");

  return static_cast<QEvent::Type>(type);
}

class CimSyncCallbackEvent final : public QEvent
{
public:
  explicit CimSyncCallbackEvent
    (std::shared_ptr<CimSyncCallbackRequest> sync_request)
    : QEvent (cim_sync_callback_event_type ()),
      request (std::move (sync_request))
  {
  }

  std::shared_ptr<CimSyncCallbackRequest> request;
};

class CimEventHandler : public QObject
{
  Q_OBJECT

public:
  CimEventHandler (CimIcHandle ic)
  {
    m_ic = ic;
  };

  ~CimEventHandler () override = default;

protected:
  bool eventFilter (QObject* obj, QEvent* event) override;

private:
  CimIcHandle m_ic;
};

bool CimEventHandler::eventFilter (QObject* obj, QEvent* event)
{
  if (m_ic != nullptr && event->type() == QEvent::MouseButtonPress)
    cim_ic_reset (m_ic);

  return QObject::eventFilter(obj, event);
}

class CimQic : public QPlatformInputContext
{
  Q_OBJECT
public:
   CimQic ();
  ~CimQic () override;

  bool isValid () const override;
  bool event (QEvent* qt_event) override;
  void reset () override;
  void commit () override;
  void update (Qt::InputMethodQueries) override;
  void invokeAction (QInputMethod::Action, int cursorPosition) override;
  bool filterEvent (const QEvent* event) override;
  QRectF keyboardRect () const override;
  bool isAnimating () const override;
  void showInputPanel () override;
  void hideInputPanel () override;
  bool isInputPanelVisible () const override;
  QLocale locale () const override;
  Qt::LayoutDirection inputDirection () const override;
  void setFocusObject (QObject* object) override;

  // cim signal callbacks
  static void cb_preedit_start   (CimIcHandle ic, void* user_data);
  static void cb_preedit_end     (CimIcHandle ic, void* user_data);
  static void cb_preedit_changed (CimIcHandle ic,
                                  const CimPreedit* preedit,
                                  void* user_data);
  static void cb_commit (CimIcHandle ic, const char* text, void* user_data);
  static const CimSurround* cb_get_surround (CimIcHandle ic, void* user_data);
  static bool cb_delete_surround (CimIcHandle ic,
                                  int32_t     offset,
                                  uint32_t    n_chars,
                                  void* user_data);
#ifdef CIM_BRIDGE_TEST
  bool test_filter_event (uint32_t keyval);
  void test_set_focus_object (QObject* object);
  bool test_wait_for_sync_pending ();
#endif
private:
  QObject* focus_object () const;
  const CimSurround* get_surround_on_owner ();
  bool delete_surround_on_owner (int32_t offset, uint32_t n_chars);
  bool run_sync_callback
    (const std::shared_ptr<CimSyncCallbackRequest>& request);
  void unregister_sync_callback (CimSyncCallbackRequest* request);

  CimIcHandle      m_ic;
  CimEventHandler* m_handler;
  std::atomic_bool m_closing;
  QThread*         m_owner_thread;
  std::mutex       m_sync_mutex;
  std::vector<CimSyncCallbackRequest*> m_sync_requests;
#ifdef CIM_BRIDGE_TEST
  std::condition_variable m_test_sync_condition;
  size_t m_test_sync_ready_count = 0;
  QObject* m_test_focus_object = nullptr;
#endif
  CimRect          m_cursor_area;
  QByteArray       m_surround_text;
  CimSurround      m_surround;
};

// cim signal callbacks
void CimQic::cb_preedit_start (CimIcHandle ic, void* user_data)
{
  (void) ic;
  (void) user_data;
}

void CimQic::cb_preedit_end (CimIcHandle ic, void* user_data)
{
  (void) ic;
  (void) user_data;
}

void CimQic::cb_preedit_changed (CimIcHandle ic,
                                 const CimPreedit* preedit,
                                 void* user_data)
{
  CimQic* context;
  CimCallbackEvent* callback_event = nullptr;
  size_t text_length;
  int cursor_utf16;

  (void) ic;

  if (!user_data || !preedit || !preedit->text ||
      (preedit->attrs_len != 0 && !preedit->attrs))
    qFatal ("Cim Qt bridge received an invalid preedit callback");

  context = static_cast<CimQic*>(user_data);
  if (context->m_closing.load (std::memory_order_acquire))
    return;

  if (!utf8_length (preedit->text, &text_length) ||
      clair_utf8_validate (preedit->text, text_length) != CLAIR_OK)
    qFatal ("Cim Qt bridge received invalid preedit UTF-8");

  if (!utf8_scalar_to_utf16
        (preedit->text, text_length, preedit->cursor_pos, &cursor_utf16))
    qFatal ("Cim Qt bridge received an invalid preedit cursor");

  for (uint32_t index = 0; index < preedit->attrs_len; index++)
  {
    const CimTextAttr& source = preedit->attrs[index];
    uint64_t end_scalar = static_cast<uint64_t>(source.pos) + source.n_chars;
    int start_utf16;
    int end_utf16;

    if (source.type != CIM_TEXT_ATTR_HIGHLIGHT &&
        source.type != CIM_TEXT_ATTR_UNDERLINE)
      qFatal ("Cim Qt bridge received an invalid preedit attribute type");

    if (end_scalar > std::numeric_limits<uint32_t>::max () ||
        !utf8_scalar_to_utf16
          (preedit->text, text_length, source.pos, &start_utf16) ||
        !utf8_scalar_to_utf16
          (preedit->text,
           text_length,
           static_cast<uint32_t>(end_scalar),
           &end_utf16))
      qFatal ("Cim Qt bridge received an invalid preedit attribute range");
  }

  try
  {
    callback_event = new CimCallbackEvent
      (CimCallbackEventKind::Preedit_Changed);
    callback_event->text.assign (preedit->text, text_length);
    callback_event->cursor_pos = preedit->cursor_pos;

    if (preedit->attrs_len != 0)
      callback_event->attrs.assign
        (preedit->attrs, preedit->attrs + preedit->attrs_len);
  }
  catch (...)
  {
    delete callback_event;
    qFatal ("Cim Qt bridge could not copy preedit callback payload");
  }

  QCoreApplication::postEvent (context, callback_event);
}

void CimQic::cb_commit (CimIcHandle ic, const char* text, void* user_data)
{
  CimQic* context;
  CimCallbackEvent* callback_event = nullptr;
  size_t text_length;

  (void) ic;

  if (!user_data || !utf8_length (text, &text_length) ||
      clair_utf8_validate (text, text_length) != CLAIR_OK)
    qFatal ("Cim Qt bridge received an invalid commit callback");

  context = static_cast<CimQic*>(user_data);
  if (context->m_closing.load (std::memory_order_acquire))
    return;

  try
  {
    callback_event = new CimCallbackEvent (CimCallbackEventKind::Commit);
    callback_event->text.assign (text, text_length);
  }
  catch (...)
  {
    delete callback_event;
    qFatal ("Cim Qt bridge could not copy commit callback payload");
  }

  QCoreApplication::postEvent (context, callback_event);
}

bool CimQic::event (QEvent* qt_event)
{
  if (qt_event->type () == cim_sync_callback_event_type ())
  {
    CimSyncCallbackEvent* sync_event =
      static_cast<CimSyncCallbackEvent*>(qt_event);
    std::shared_ptr<CimSyncCallbackRequest> request = sync_event->request;
    bool result = false;

    if (!m_closing.load (std::memory_order_acquire))
    {
      if (request->kind == CimSyncCallbackKind::Get_Surround)
        result = get_surround_on_owner () != nullptr;
      else
        result = delete_surround_on_owner
          (request->offset, request->n_chars);
    }

    {
      std::lock_guard<std::mutex> lock (request->mutex);

      if (!request->completed)
      {
        request->result = result;
        request->completed = true;
      }
    }

    request->condition.notify_all ();
    return true;
  }

  if (qt_event->type () != cim_callback_event_type ())
    return QPlatformInputContext::event (qt_event);

  if (m_closing.load (std::memory_order_acquire))
    return true;

  CimCallbackEvent* callback_event =
    static_cast<CimCallbackEvent*>(qt_event);
  QObject* object = focus_object ();

  if (!object)
    return true;

  if (callback_event->kind == CimCallbackEventKind::Commit)
  {
    QString text = QString::fromUtf8
      (callback_event->text.data (),
       static_cast<qsizetype>(callback_event->text.size ()));
    QInputMethodEvent input_event;
    input_event.setCommitString (text);
    QCoreApplication::sendEvent (object, &input_event);
    return true;
  }

  QString preedit_text = QString::fromUtf8
    (callback_event->text.data (),
     static_cast<qsizetype>(callback_event->text.size ()));
  QList<QInputMethodEvent::Attribute> attrs;

  for (const CimTextAttr& source : callback_event->attrs)
  {
    uint64_t end_scalar = static_cast<uint64_t>(source.pos) + source.n_chars;
    int start_utf16;
    int end_utf16;
    QTextCharFormat format;

    if (end_scalar > std::numeric_limits<uint32_t>::max () ||
        !utf8_scalar_to_utf16
          (callback_event->text.data (),
           callback_event->text.size (),
           source.pos,
           &start_utf16) ||
        !utf8_scalar_to_utf16
          (callback_event->text.data (),
           callback_event->text.size (),
           static_cast<uint32_t>(end_scalar),
           &end_utf16))
      qFatal ("Cim Qt bridge lost a validated preedit attribute range");

    switch (source.type)
    {
      case CIM_TEXT_ATTR_HIGHLIGHT:
        format.setBackground (Qt::green);
        format.setForeground (Qt::black);
        break;
      case CIM_TEXT_ATTR_UNDERLINE:
        format.setUnderlineStyle (QTextCharFormat::DashUnderline);
        break;
      default:
        qFatal ("Cim Qt bridge lost a validated preedit attribute type");
    }

    attrs << QInputMethodEvent::Attribute
      (QInputMethodEvent::TextFormat,
       start_utf16,
       end_utf16 - start_utf16,
       format);
  }

  int cursor_utf16;
  if (!utf8_scalar_to_utf16
        (callback_event->text.data (),
         callback_event->text.size (),
         callback_event->cursor_pos,
         &cursor_utf16))
    qFatal ("Cim Qt bridge lost a validated preedit cursor");

  attrs << QInputMethodEvent::Attribute
    (QInputMethodEvent::Cursor, cursor_utf16, 1, QVariant ());

  QInputMethodEvent input_event (preedit_text, attrs);
  QCoreApplication::sendEvent (object, &input_event);
  return true;
}

QObject* CimQic::focus_object () const
{
#ifdef CIM_BRIDGE_TEST
  if (m_test_focus_object != nullptr)
    return m_test_focus_object;
#endif

  return qApp->focusObject ();
}

const CimSurround* CimQic::get_surround_on_owner ()
{
  QObject* object = focus_object ();

  if (!object)
    return nullptr;

  QInputMethodQueryEvent surround_query (Qt::ImSurroundingText);
  QInputMethodQueryEvent cursor_query   (Qt::ImCursorPosition);
  QInputMethodQueryEvent anchor_query   (Qt::ImAnchorPosition);

  QCoreApplication::sendEvent (object, &surround_query);
  QCoreApplication::sendEvent (object, &cursor_query);
  QCoreApplication::sendEvent (object, &anchor_query);

  QString string = surround_query.value (Qt::ImSurroundingText).toString();
  int cursor_utf16 = cursor_query.value (Qt::ImCursorPosition).toInt();
  int anchor_utf16 = anchor_query.value (Qt::ImAnchorPosition).toInt();

  m_surround_text = string.toUtf8 ();

  qsizetype byte_length = m_surround_text.size ();
  if (byte_length < 0 ||
      static_cast<uint64_t>(byte_length) >
        std::numeric_limits<uint32_t>::max())
    return nullptr;

  size_t text_length = static_cast<size_t>(byte_length);
  uint32_t cursor_pos;
  uint32_t anchor_pos;

  if (!utf8_utf16_to_scalar
        (m_surround_text.constData (),
         text_length,
         cursor_utf16,
         &cursor_pos) ||
      !utf8_utf16_to_scalar
        (m_surround_text.constData (),
         text_length,
         anchor_utf16,
         &anchor_pos))
    return nullptr;

  m_surround.text = m_surround_text.constData ();
  m_surround.len = static_cast<uint32_t>(text_length);
  m_surround.cursor_pos = cursor_pos;
  m_surround.anchor_pos = anchor_pos;

  return &m_surround;
}

bool CimQic::delete_surround_on_owner (int32_t offset, uint32_t n_chars)
{
  QObject* object = focus_object ();

  if (!object)
    return false;

  const CimSurround* surround = get_surround_on_owner ();
  if (surround == nullptr)
    return false;

  int64_t start_scalar = static_cast<int64_t>(surround->cursor_pos) + offset;
  if (start_scalar < 0)
    return false;

  uint64_t end_scalar = static_cast<uint64_t>(start_scalar) + n_chars;
  if (end_scalar > std::numeric_limits<uint32_t>::max())
    return false;

  int cursor_utf16;
  int start_utf16;
  int end_utf16;

  if (!utf8_scalar_to_utf16
        (m_surround.text,
         m_surround.len,
         m_surround.cursor_pos,
         &cursor_utf16) ||
      !utf8_scalar_to_utf16
        (m_surround.text,
         m_surround.len,
         static_cast<uint32_t>(start_scalar),
         &start_utf16) ||
      !utf8_scalar_to_utf16
        (m_surround.text,
         m_surround.len,
         static_cast<uint32_t>(end_scalar),
         &end_utf16))
    return false;

  int64_t replace_from = static_cast<int64_t>(start_utf16) - cursor_utf16;
  int64_t replace_length = static_cast<int64_t>(end_utf16) - start_utf16;
  if (replace_from < std::numeric_limits<int>::min() ||
      replace_from > std::numeric_limits<int>::max() ||
      replace_length < 0 ||
      replace_length > std::numeric_limits<int>::max())
    return false;

  QInputMethodEvent event;
  event.setCommitString
    ("", static_cast<int>(replace_from), static_cast<int>(replace_length));
  QCoreApplication::sendEvent (object, &event);

  return true;
}

void CimQic::unregister_sync_callback (CimSyncCallbackRequest* request)
{
  std::lock_guard<std::mutex> lock (m_sync_mutex);

  for (auto iterator = m_sync_requests.begin ();
       iterator != m_sync_requests.end ();
       ++iterator)
  {
    if (*iterator != request)
      continue;

#ifdef CIM_BRIDGE_TEST
    if (request->test_ready)
    {
      if (m_test_sync_ready_count == 0)
        qFatal ("Cim Qt bridge test lost a synchronous callback");

      m_test_sync_ready_count--;
    }
#endif
    m_sync_requests.erase (iterator);
    return;
  }

  qFatal ("Cim Qt bridge lost a synchronous callback registration");
}

bool CimQic::run_sync_callback
  (const std::shared_ptr<CimSyncCallbackRequest>& request)
{
  {
    std::lock_guard<std::mutex> lock (m_sync_mutex);

    if (m_closing.load (std::memory_order_acquire))
      return false;

    m_sync_requests.push_back (request.get ());
  }

  CimSyncCallbackEvent* sync_event = nullptr;

  try
  {
    sync_event = new CimSyncCallbackEvent (request);
  }
  catch (...)
  {
    unregister_sync_callback (request.get ());
    qFatal ("Cim Qt bridge could not allocate a synchronous callback event");
  }

  QCoreApplication::postEvent (this, sync_event);

#ifdef CIM_BRIDGE_TEST
  {
    std::lock_guard<std::mutex> lock (m_sync_mutex);
    request->test_ready = true;
    m_test_sync_ready_count++;
  }
  m_test_sync_condition.notify_all ();
#endif

  bool result;
  {
    std::unique_lock<std::mutex> lock (request->mutex);
    request->condition.wait (lock, [&request] { return request->completed; });
    result = request->result;
  }

  unregister_sync_callback (request.get ());
  return result;
}

const CimSurround*
CimQic::cb_get_surround (CimIcHandle ic, void* user_data)
{
  CimQic* context;

  (void) ic;

  if (!user_data)
    qFatal ("Cim Qt bridge received invalid callback user data");

  context = static_cast<CimQic*>(user_data);

  if (QThread::currentThread () == context->m_owner_thread)
  {
    if (context->m_closing.load (std::memory_order_acquire))
      return nullptr;

    return context->get_surround_on_owner ();
  }

  std::shared_ptr<CimSyncCallbackRequest> request;

  try
  {
    request = std::make_shared<CimSyncCallbackRequest>
      (CimSyncCallbackKind::Get_Surround);
  }
  catch (...)
  {
    qFatal ("Cim Qt bridge could not allocate a synchronous callback");
  }

  if (context->run_sync_callback (request))
    return &context->m_surround;

  return nullptr;
}

bool CimQic::cb_delete_surround (CimIcHandle ic,
                                 int32_t offset,
                                 uint32_t n_chars,
                                 void* user_data)
{
  CimQic* context;

  (void) ic;

  if (!user_data)
    qFatal ("Cim Qt bridge received invalid callback user data");

  context = static_cast<CimQic*>(user_data);

  if (QThread::currentThread () == context->m_owner_thread)
  {
    if (context->m_closing.load (std::memory_order_acquire))
      return false;

    return context->delete_surround_on_owner (offset, n_chars);
  }

  std::shared_ptr<CimSyncCallbackRequest> request;

  try
  {
    request = std::make_shared<CimSyncCallbackRequest>
      (CimSyncCallbackKind::Delete_Surround);
  }
  catch (...)
  {
    qFatal ("Cim Qt bridge could not allocate a synchronous callback");
  }

  request->offset = offset;
  request->n_chars = n_chars;
  return context->run_sync_callback (request);
}

static CimCallbacks callbacks = {
  .preedit_start   = CimQic::cb_preedit_start,
  .preedit_end     = CimQic::cb_preedit_end,
  .preedit_changed = CimQic::cb_preedit_changed,
  .commit          = CimQic::cb_commit,
  .get_surround    = CimQic::cb_get_surround,
  .delete_surround = CimQic::cb_delete_surround
};

#ifdef CIM_BRIDGE_TEST
bool CimQic::test_filter_event (uint32_t keyval)
{
  CimEvent event = {};

  if (m_ic == nullptr)
    qFatal ("Cim Qt bridge test input context is unavailable");

  event.type = CIM_EVENT_KEY_PRESS;
  event.keyval = keyval;
  return cim_ic_filter_event (m_ic, &event);
}

void CimQic::test_set_focus_object (QObject* object)
{
  m_test_focus_object = object;
}

bool CimQic::test_wait_for_sync_pending ()
{
  std::unique_lock<std::mutex> lock (m_sync_mutex);
  bool signaled = m_test_sync_condition.wait_for
    (lock,
     std::chrono::seconds (5),
     [this]
     {
       return m_test_sync_ready_count != 0 ||
              m_closing.load (std::memory_order_acquire);
     });

  return signaled && m_test_sync_ready_count != 0;
}

QPlatformInputContext*
cim_qic_test_create ()
{
  return new CimQic ();
}

bool
cim_qic_test_filter_event (QPlatformInputContext* context, uint32_t keyval)
{
  if (context == nullptr)
    qFatal ("Cim Qt bridge test received a NULL context");

  return static_cast<CimQic*>(context)->test_filter_event (keyval);
}

void
cim_qic_test_set_focus_object (QPlatformInputContext* context,
                               QObject* object)
{
  if (context == nullptr)
    qFatal ("Cim Qt bridge test received a NULL context");

  static_cast<CimQic*>(context)->test_set_focus_object (object);
}

bool
cim_qic_test_wait_for_sync_pending (QPlatformInputContext* context)
{
  if (context == nullptr)
    qFatal ("Cim Qt bridge test received a NULL context");

  return static_cast<CimQic*>(context)->test_wait_for_sync_pending ();
}

void
cim_qic_test_invoke_commit (QPlatformInputContext* context,
                            const char* text)
{
  if (context == nullptr || text == nullptr)
    qFatal ("Cim Qt bridge test received invalid commit input");

  CimQic* cim_context = static_cast<CimQic*>(context);

  if (!cim_context->isValid ())
    qFatal ("Cim Qt bridge test commit callback is unavailable");

  CimQic::cb_commit (nullptr, text, cim_context);
}

bool
cim_qic_test_get_commit_event (const QEvent* event, std::string* text)
{
  if (event == nullptr || text == nullptr ||
      event->type () != cim_callback_event_type ())
    return false;

  const CimCallbackEvent* callback_event =
    static_cast<const CimCallbackEvent*>(event);

  if (callback_event->kind != CimCallbackEventKind::Commit)
    return false;

  *text = callback_event->text;
  return true;
}
#endif

CimQic::CimQic ()
{
  m_ic                  = nullptr;
  m_handler             = nullptr;
  m_closing.store (false, std::memory_order_relaxed);
  m_owner_thread        = QThread::currentThread ();
  m_cursor_area.x       = 0;
  m_cursor_area.y       = 0;
  m_cursor_area.width   = 0;
  m_cursor_area.height  = 0;
  m_surround.text       = nullptr;
  m_surround.len        = 0;
  m_surround.cursor_pos = 0;
  m_surround.anchor_pos = 0;

  m_ic = cim_ic_create ();

  if (m_ic != nullptr)
    cim_ic_set_callbacks (m_ic, &callbacks, this);
}

CimQic::~CimQic ()
{
  {
    std::lock_guard<std::mutex> lock (m_sync_mutex);

    m_closing.store (true, std::memory_order_release);

    for (CimSyncCallbackRequest* request : m_sync_requests)
    {
      std::lock_guard<std::mutex> request_lock (request->mutex);

      if (!request->completed)
      {
        request->result = false;
        request->completed = true;
        request->condition.notify_all ();
      }
    }
  }

  if (m_ic)
  {
    cim_ic_destroy (m_ic);
    m_ic = nullptr;
  }

  {
    std::lock_guard<std::mutex> lock (m_sync_mutex);

    if (!m_sync_requests.empty ())
      qFatal ("Cim Qt plugin destroy did not quiesce callbacks");
  }

  if (m_handler)
    delete m_handler;
}

bool CimQic::isValid () const
{
  if (m_ic == nullptr)
    return false;

  return true;
}

void CimQic::reset ()
{
  if (m_ic != nullptr)
    cim_ic_reset (m_ic);
}

void CimQic::commit ()
{
  if (m_ic != nullptr)
    cim_ic_reset (m_ic);
}

void CimQic::update (Qt::InputMethodQueries queries)
{
  if (m_ic == nullptr)
    return;

  if (queries & Qt::ImCursorRectangle)
  {
    QWidget* widget = qApp->focusWidget ();

    if (widget == nullptr)
      return;

    QRect  rect  = widget->inputMethodQuery(Qt::ImCursorRectangle).toRect();
    QPoint point = widget->mapToGlobal (QPoint (0, 0));
    rect.translate (point);

    CimRect cursor_area;
    if (!qrect_to_cim (rect, &cursor_area))
      qFatal ("Cim Qt bridge received an invalid cursor rectangle");

    if (m_cursor_area.x      != cursor_area.x      ||
        m_cursor_area.y      != cursor_area.y      ||
        m_cursor_area.width  != cursor_area.width  ||
        m_cursor_area.height != cursor_area.height)
    {
      m_cursor_area = cursor_area;

      cim_ic_set_cursor_pos (m_ic, &m_cursor_area);
    }
  }
}

void CimQic::invokeAction (QInputMethod::Action, int cursorPosition)
{
}

bool CimQic::filterEvent (const QEvent* event)
{
  if (m_ic == nullptr)
    return false;

  if (!qApp->focusObject() || !inputMethodAccepted())
    return false;

  bool  retval;
  const QKeyEvent* key_event = static_cast<const QKeyEvent*>(event);
  CimEvent cevent;

  switch (event->type ())
  {
#undef KeyPress
    case QEvent::KeyPress:
      cevent.type = CIM_EVENT_KEY_PRESS;
      break;
#undef KeyRelease
    case QEvent::KeyRelease:
      cevent.type = CIM_EVENT_KEY_RELEASE;
      break;
    default:
      return false;
  }

  cevent.state   = key_event->nativeModifiers  ();
  cevent.keyval  = key_event->nativeVirtualKey ();
  cevent.keycode = key_event->nativeScanCode   ();

  retval = cim_ic_filter_event (m_ic, &cevent);

  return retval;
}

QRectF CimQic::keyboardRect() const
{
  return QRectF ();
}

bool CimQic::isAnimating() const
{
  return false;
}

void CimQic::showInputPanel()
{
}

void CimQic::hideInputPanel()
{
}

bool CimQic::isInputPanelVisible() const
{
  return false;
}

QLocale CimQic::locale() const
{
  return QLocale ();
}

Qt::LayoutDirection CimQic::inputDirection() const
{
  return Qt::LayoutDirection ();
}

void CimQic::setFocusObject (QObject* object)
{
  if (m_ic != nullptr && (!object || !inputMethodAccepted()))
    cim_ic_focus_out (m_ic);

  QPlatformInputContext::setFocusObject (object);

  if (m_ic != nullptr && object && inputMethodAccepted())
    cim_ic_focus_in (m_ic);

  update (Qt::ImCursorRectangle);
}

/*
  class CimQicPlugin
 */
class CimQicPlugin : public QPlatformInputContextPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID
    QPlatformInputContextFactoryInterface_iid
    FILE "./cim.json")

public:
  CimQicPlugin ()
  {
  }

  ~CimQicPlugin () override = default;

  QPlatformInputContext* create (const QString     &key,
                                 const QStringList &paramList) override
  {
    return new CimQic ();
  }
};

#include "im-cim-qt.moc"
