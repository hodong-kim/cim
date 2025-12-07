require 'fiddle'
require 'fiddle/import'

module LibCim
  extend Fiddle::Importer
  dlload './libcim.so'

  CimEvent = struct ['int     type',
                     'int32_t state',
                     'int32_t keyval',
                     'int32_t keycode']

  extern 'CimIc* cim_ic_new ()'
  @cim_ic_free = extern 'void   cim_ic_free (CimIc* ic)'
  extern 'void   cim_ic_focus_in       (CimIc* ic)'
  extern 'void   cim_ic_focus_out      (CimIc* ic)'
  extern 'void   cim_ic_reset          (CimIc* ic)'
  # There is no Fiddle::TYPE_BOOL. So I changed it to char type.
  extern 'char   cim_ic_filter_event   (CimIc* ic, const CimEvent* event)'
  extern 'void   cim_ic_set_cursor_pos (CimIc* ic, const CimRect*  area)'
  extern 'void   cim_ic_set_callbacks  (CimIc* ic, ...)'
  extern 'const CimPreedit*   cim_ic_get_preedit   (CimIc* ic)'
  extern 'const CimCandidate* cim_ic_get_candidate (CimIc* ic)'
  extern 'void  cim_ic_activate_candidate_item (CimIc* ic, int row, int col)'
  extern 'void  cim_ic_change_candidate_page   (CimIc* ic, int page_index)'

  def self.cim_ic_free
    return @cim_ic_free
  end
end

module Cim
  class Event
    attr_reader :cstruct

    def initialize(type=0, state=0, keyval=0, keycode=0)
      @cstruct = LibCim::CimEvent.malloc(Fiddle::RUBY_FREE)
      @cstruct.type    = type
      @cstruct.state   = state
      @cstruct.keyval  = keyval
      @cstruct.keycode = keycode
    end

    def type=(arg)
      @cstruct.type = arg
    end

    def state=(arg)
      @cstruct.state = arg
    end

    def keyval=(arg)
      @cstruct.keyval = arg
    end

    def keycode=(arg)
      @cstruct.keycode = arg
    end
  end

  class Ic
    def initialize
      @ic = LibCim.cim_ic_new
      # The cim_ic_free function is automatically called
      # when the GC collects garbage.
      @ic.free = LibCim.cim_ic_free
    end

    def focus_in
      LibCim.cim_ic_focus_in(@ic)
    end

    def focus_out
      LibCim.cim_ic_focus_out(@ic)
    end

    def filter_event(event)
      return LibCim.cim_ic_filter_event(@ic, event.cstruct)
    end

    def set_callbacks(*args)
      new_args = []
      new_args << @ic
      args.each_with_index do |arg, i|
        case (i % 3)
        when 0 # type
          new_args << Fiddle::TYPE_INT
          new_args << arg
        when 1 # func
          closure = Class.new(Fiddle::Closure) {

            attr_writer :callback, :ic

            def call(ic, text, user_data)
              @callback.call(@ic, text, user_data)
            end
          }.new(Fiddle::TYPE_VOID, [Fiddle::TYPE_VOIDP,
                                    Fiddle::TYPE_VOIDP,
                                    Fiddle::TYPE_VOIDP])
          closure.callback = arg
          closure.ic       = self
          func = Fiddle::Function.new(closure,
                                      [Fiddle::TYPE_VOIDP,
                                       Fiddle::TYPE_VOIDP,
                                       Fiddle::TYPE_VOIDP],
                                      Fiddle::TYPE_VOID)
          new_args << Fiddle::TYPE_VOIDP
          new_args << func
        when 2 # user_data
          new_args << Fiddle::TYPE_VOIDP
          new_args << arg
        end
      end
      new_args << Fiddle::TYPE_INT
      new_args << -1
      LibCim.cim_ic_set_callbacks(*new_args)
    end
  end
end

def cb_commit(ic, text, user_data)
  puts text
end

CIM_CB_COMMIT = 3

10000.times do
  puts "Try  Cim::Ic.new"
  ic = Cim::Ic.new
  puts "Done Cim::Ic.new"
  puts "Try  set_callbacks"
  ic.set_callbacks(CIM_CB_COMMIT, method(:cb_commit), 0)
  puts "Done set_callbacks"
  puts "Try  focus_in"
  ic.focus_in
  puts "Done focus_in"
  event = Cim::Event.new(0, 0, 0xff31, 108)
  puts ">> filter_event"
  ic.filter_event(event)
  puts "<< filter_event"

  event.keyval  = 'd'.ord
  event.keycode = 40
  ic.filter_event(event)
  event.keyval  = 'k'.ord
  event.keycode = 45
  ic.filter_event(event)
  event.keyval  = 's'.ord
  event.keycode = 39
  ic.filter_event(event)

  event.keyval  = 's'.ord
  event.keycode = 39
  ic.filter_event(event)
  event.keyval  = 'u'.ord
  event.keycode = 30
  ic.filter_event(event)
  event.keyval  = 'd'.ord
  event.keycode = 40
  ic.filter_event(event)

  ic.focus_out
end
