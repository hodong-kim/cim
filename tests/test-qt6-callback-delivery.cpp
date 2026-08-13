#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <QByteArray>
#include <QCoreApplication>
#include <QEvent>
#include <QEventLoop>
#include <QInputMethodEvent>
#include <QObject>
#include <QThread>
#include <QtGui/qpa/qplatforminputcontext.h>

QPlatformInputContext* cim_qic_test_create ();
bool cim_qic_test_filter_event
  (QPlatformInputContext* context, uint32_t keyval);
void cim_qic_test_set_focus_object (QPlatformInputContext* context,
                                    QObject* object);
bool cim_qic_test_wait_for_sync_pending (QPlatformInputContext* context);
void cim_qic_test_invoke_commit (QPlatformInputContext* context,
                                 const char* text);
bool cim_qic_test_get_commit_event (const QEvent* event,
                                    std::string* text);

class CommitEventFilter final : public QObject
{
public:
  explicit CommitEventFilter (QThread* owner_thread)
    : owner_thread_ (owner_thread)
  {
  }

  int commit_count () const
  {
    return commit_count_;
  }

  bool delivered_on_owner () const
  {
    return delivered_on_owner_;
  }

  const std::string& text () const
  {
    return text_;
  }

protected:
  bool eventFilter (QObject* object, QEvent* event) override
  {
    std::string text;

    (void) object;

    if (!cim_qic_test_get_commit_event (event, &text))
      return false;

    commit_count_++;
    delivered_on_owner_ = QThread::currentThread () == owner_thread_;
    text_ = std::move (text);
    return true;
  }

private:
  QThread* owner_thread_ = nullptr;
  int commit_count_ = 0;
  bool delivered_on_owner_ = false;
  std::string text_;
};


class SurroundObject final : public QObject
{
public:
  explicit SurroundObject (QThread* owner_thread)
    : owner_thread_ (owner_thread)
  {
  }

  int query_count () const
  {
    return query_count_;
  }

  bool query_on_owner () const
  {
    return query_on_owner_;
  }

  int delete_count () const
  {
    return delete_count_;
  }

  bool delete_on_owner () const
  {
    return delete_on_owner_;
  }

  int replacement_start () const
  {
    return replacement_start_;
  }

  int replacement_length () const
  {
    return replacement_length_;
  }

protected:
  bool event (QEvent* event) override
  {
    static const char text[] = "A\xF0\x9F\x98\x80" "B";

    if (event->type () == QEvent::InputMethodQuery)
    {
      auto* query_event = static_cast<QInputMethodQueryEvent*>(event);
      Qt::InputMethodQueries queries = query_event->queries ();

      query_count_++;
      if (QThread::currentThread () != owner_thread_)
        query_on_owner_ = false;

      if (queries.testFlag (Qt::ImSurroundingText))
        query_event->setValue
          (Qt::ImSurroundingText, QString::fromUtf8 (text));
      if (queries.testFlag (Qt::ImCursorPosition))
        query_event->setValue (Qt::ImCursorPosition, 3);
      if (queries.testFlag (Qt::ImAnchorPosition))
        query_event->setValue (Qt::ImAnchorPosition, 3);
      return true;
    }

    if (event->type () == QEvent::InputMethod)
    {
      auto* input_event = static_cast<QInputMethodEvent*>(event);

      delete_count_++;
      if (QThread::currentThread () != owner_thread_)
        delete_on_owner_ = false;

      replacement_start_ = input_event->replacementStart ();
      replacement_length_ = input_event->replacementLength ();
      return true;
    }

    return QObject::event (event);
  }

private:
  QThread* owner_thread_ = nullptr;
  int query_count_ = 0;
  bool query_on_owner_ = true;
  int delete_count_ = 0;
  bool delete_on_owner_ = true;
  int replacement_start_ = 0;
  int replacement_length_ = 0;
};

class PreeditObject final : public QObject
{
public:
  explicit PreeditObject (QThread* owner_thread)
    : owner_thread_ (owner_thread)
  {
  }

  int preedit_count () const
  {
    return preedit_count_;
  }

  bool delivered_on_owner () const
  {
    return delivered_on_owner_;
  }

  bool payload_valid () const
  {
    return payload_valid_;
  }

protected:
  bool event (QEvent* event) override
  {
    static const char expected[] = "A\xF0\x9F\x98\x80" "B";

    if (event->type () != QEvent::InputMethod)
      return QObject::event (event);

    auto* input_event = static_cast<QInputMethodEvent*>(event);
    bool highlight_range = false;
    bool underline_range = false;
    bool cursor_range = false;

    preedit_count_++;
    delivered_on_owner_ = QThread::currentThread () == owner_thread_;

    for (const QInputMethodEvent::Attribute& attr :
         input_event->attributes ())
    {
      if (attr.type == QInputMethodEvent::TextFormat &&
          attr.start == 0 && attr.length == 1)
        highlight_range = true;
      else if (attr.type == QInputMethodEvent::TextFormat &&
               attr.start == 1 && attr.length == 2)
        underline_range = true;
      else if (attr.type == QInputMethodEvent::Cursor &&
               attr.start == 3 && attr.length == 1)
        cursor_range = true;
    }

    payload_valid_ =
      input_event->preeditString () == QString::fromUtf8 (expected) &&
      input_event->attributes ().size () == 3 &&
      highlight_range && underline_range && cursor_range;
    return true;
  }

private:
  QThread* owner_thread_ = nullptr;
  int preedit_count_ = 0;
  bool delivered_on_owner_ = false;
  bool payload_valid_ = false;
};

static bool
set_plugin (const char* plugin_name)
{
  QByteArray plugin_dir = qgetenv ("CIM_TEST_PLUGIN_DIR");

  if (plugin_dir.isEmpty () || plugin_name == nullptr ||
      plugin_name[0] == '\0')
  {
    std::cerr << "Qt callback test plugin is not configured\n";
    return false;
  }

  QByteArray plugin_path = plugin_dir + "/" + plugin_name;
  if (!qputenv ("CIM_PLUGIN", plugin_path))
  {
    std::cerr << "could not configure CIM_PLUGIN\n";
    return false;
  }

  return true;
}

static bool
run_delivery_case ()
{
  static const std::string expected = "cross-thread-owned-payload";
  QPlatformInputContext* context = cim_qic_test_create ();

  if (context == nullptr || !context->isValid ())
  {
    std::cerr << "could not create Qt Cim context\n";
    delete context;
    return false;
  }

  CommitEventFilter filter (QThread::currentThread ());
  context->installEventFilter (&filter);

  char payload[] = "cross-thread-owned-payload";
  std::thread worker ([&context, &payload] {
    cim_qic_test_invoke_commit (context, payload);
    std::memset (payload, 'x', sizeof (payload) - 1);
  });
  worker.join ();

  QCoreApplication::processEvents ();

  bool passed =
    filter.commit_count () == 1 &&
    filter.delivered_on_owner () &&
    filter.text () == expected;

  if (!passed)
    std::cerr << "Qt cross-thread callback delivery failed\n";

  delete context;
  return passed;
}

static bool
run_pending_teardown_case ()
{
  QPlatformInputContext* context = cim_qic_test_create ();

  if (context == nullptr || !context->isValid ())
  {
    std::cerr << "could not create Qt Cim context\n";
    delete context;
    return false;
  }

  CommitEventFilter filter (QThread::currentThread ());
  context->installEventFilter (&filter);

  char payload[] = "must-not-be-delivered";
  std::thread worker ([&context, &payload] {
    cim_qic_test_invoke_commit (context, payload);
    std::memset (payload, 'x', sizeof (payload) - 1);
  });
  worker.join ();

  delete context;
  QCoreApplication::processEvents ();

  if (filter.commit_count () != 0)
  {
    std::cerr << "Qt pending callback survived teardown\n";
    return false;
  }

  return true;
}

static void
process_until_commit (CommitEventFilter& filter)
{
  while (filter.commit_count () == 0)
    QCoreApplication::processEvents
      (QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents);
}

static bool
run_get_surround_case ()
{
  QPlatformInputContext* context = cim_qic_test_create ();

  if (context == nullptr || !context->isValid ())
  {
    std::cerr << "could not create Qt Cim context\n";
    delete context;
    return false;
  }

  CommitEventFilter filter (QThread::currentThread ());
  SurroundObject surround (QThread::currentThread ());
  context->installEventFilter (&filter);
  cim_qic_test_set_focus_object (context, &surround);

  if (!cim_qic_test_filter_event (context, 'G') ||
      !cim_qic_test_wait_for_sync_pending (context))
  {
    std::cerr << "Qt get-surround callback was not queued\n";
    delete context;
    return false;
  }

  process_until_commit (filter);

  bool passed =
    filter.commit_count () == 1 &&
    filter.delivered_on_owner () &&
    filter.text () == "get-surround-ok" &&
    surround.query_count () == 3 &&
    surround.query_on_owner ();

  if (!passed)
    std::cerr << "Qt synchronous get-surround callback failed\n";

  delete context;
  return passed;
}

static bool
run_delete_surround_case ()
{
  QPlatformInputContext* context = cim_qic_test_create ();

  if (context == nullptr || !context->isValid ())
  {
    std::cerr << "could not create Qt Cim context\n";
    delete context;
    return false;
  }

  CommitEventFilter filter (QThread::currentThread ());
  SurroundObject surround (QThread::currentThread ());
  context->installEventFilter (&filter);
  cim_qic_test_set_focus_object (context, &surround);

  if (!cim_qic_test_filter_event (context, 'D') ||
      !cim_qic_test_wait_for_sync_pending (context))
  {
    std::cerr << "Qt delete-surround callback was not queued\n";
    delete context;
    return false;
  }

  process_until_commit (filter);

  bool passed =
    filter.commit_count () == 1 &&
    filter.delivered_on_owner () &&
    filter.text () == "delete-surround-ok" &&
    surround.query_count () == 3 &&
    surround.query_on_owner () &&
    surround.delete_count () == 1 &&
    surround.delete_on_owner () &&
    surround.replacement_start () == -2 &&
    surround.replacement_length () == 2;

  if (!passed)
    std::cerr << "Qt synchronous delete-surround callback failed\n";

  delete context;
  return passed;
}

static bool
run_sync_teardown_case ()
{
  QPlatformInputContext* context = cim_qic_test_create ();

  if (context == nullptr || !context->isValid ())
  {
    std::cerr << "could not create Qt Cim context\n";
    delete context;
    return false;
  }

  if (!cim_qic_test_filter_event (context, 'T') ||
      !cim_qic_test_wait_for_sync_pending (context))
  {
    std::cerr << "Qt teardown callback was not left pending\n";
    delete context;
    return false;
  }

  delete context;
  QCoreApplication::processEvents ();
  return true;
}

static bool
run_multiple_preedit_context_case ()
{
  QPlatformInputContext* first = cim_qic_test_create ();
  QPlatformInputContext* second = cim_qic_test_create ();

  if (first == nullptr || second == nullptr ||
      !first->isValid () || !second->isValid ())
  {
    std::cerr << "could not create simultaneous Qt contexts\n";
    delete second;
    delete first;
    return false;
  }

  CommitEventFilter first_filter (QThread::currentThread ());
  CommitEventFilter second_filter (QThread::currentThread ());
  PreeditObject first_preedit (QThread::currentThread ());
  PreeditObject second_preedit (QThread::currentThread ());

  first->installEventFilter (&first_filter);
  second->installEventFilter (&second_filter);
  cim_qic_test_set_focus_object (first, &first_preedit);
  cim_qic_test_set_focus_object (second, &second_preedit);

  if (!cim_qic_test_filter_event (first, 'P') ||
      !cim_qic_test_filter_event (second, 'P'))
  {
    std::cerr << "Qt simultaneous preedit callbacks did not start\n";
    delete second;
    delete first;
    return false;
  }

  while (first_filter.commit_count () == 0 ||
         second_filter.commit_count () == 0)
  {
    QCoreApplication::processEvents
      (QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents);
  }

  QCoreApplication::processEvents ();

  bool passed =
    first_filter.commit_count () == 1 &&
    first_filter.delivered_on_owner () &&
    first_filter.text () == "preedit-sent" &&
    first_preedit.preedit_count () == 1 &&
    first_preedit.delivered_on_owner () &&
    first_preedit.payload_valid () &&
    second_filter.commit_count () == 1 &&
    second_filter.delivered_on_owner () &&
    second_filter.text () == "preedit-sent" &&
    second_preedit.preedit_count () == 1 &&
    second_preedit.delivered_on_owner () &&
    second_preedit.payload_valid ();

  if (!passed)
    std::cerr << "Qt simultaneous preedit contexts failed\n";

  delete second;
  delete first;
  return passed;
}

static bool
run_creation_failure_case ()
{
  QPlatformInputContext* context = cim_qic_test_create ();

  if (context == nullptr)
  {
    std::cerr << "Qt create-failure context was not created\n";
    return false;
  }

  if (context->isValid ())
  {
    std::cerr << "Qt create-failure context is unexpectedly valid\n";
    delete context;
    return false;
  }

  context->reset ();
  context->commit ();
  context->update (Qt::ImCursorRectangle);

  QEvent event (QEvent::None);
  if (context->filterEvent (&event))
  {
    std::cerr << "Qt invalid context unexpectedly filtered an event\n";
    delete context;
    return false;
  }

  if (context->isValid ())
  {
    std::cerr << "Qt invalid context changed validity after use\n";
    delete context;
    return false;
  }

  delete context;
  return true;
}

int
main (int argc, char** argv)
{
  if (!set_plugin ("im-dummy.so"))
    return 1;

  QCoreApplication application (argc, argv);

  if (!run_delivery_case () || !run_pending_teardown_case ())
    return 1;

  if (!set_plugin ("im-bridge-callback.so") ||
      !run_get_surround_case () ||
      !run_delete_surround_case () ||
      !run_multiple_preedit_context_case () ||
      !run_sync_teardown_case ())
    return 1;

  if (!set_plugin ("im-create-fail.so") ||
      !run_creation_failure_case ())
    return 1;

  qunsetenv ("CIM_PLUGIN");
  std::cout << "Qt callback delivery tests passed\n";
  return 0;
}
