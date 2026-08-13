# Rakefile
#
# Build automation script for the Cim project (Ada/C hybrid core)
#

require "rake/clean"
require "fileutils"
require "rbconfig"
require "English"
require "shellwords"
require "open3"

SRC_TOP = __dir__

def fail_config(message)
  abort "ERROR: #{message}"
end

def env_choice(primary, alias_name = nil, default = nil)
  primary_value = ENV[primary]
  alias_value   = alias_name ? ENV[alias_name] : nil

  if primary_value && alias_value && primary_value != alias_value
    fail_config "#{primary} and #{alias_name} disagree"
  end

  primary_value || alias_value || default
end

def command_words(value)
  words = Shellwords.split(value.to_s)
  fail_config "empty command" if words.empty?
  words
end

def capture_command(*command)
  stdout, stderr, status = Open3.capture3(*command)
  return stdout.strip if status.success?

  fail_config "command failed: #{command.join(' ')}\n#{stderr}"
end

def find_program(name)
  return name if name.include?(File::SEPARATOR) && File.executable?(name)

  ENV.fetch("PATH", "").split(File::PATH_SEPARATOR).each do |dir|
    path = File.join(dir, name)
    return path if File.executable?(path)
  end

  nil
end

def command_available?(name)
  !find_program(name).nil?
end

def infer_target_os(target)
  value = target.to_s.downcase

  return "android" if value.include?("android")
  return "freebsd" if value.include?("freebsd")
  return "linux"   if value.include?("linux")
  return "darwin"  if value.include?("darwin") || value.include?("apple")
  return "windows" if value.include?("mingw") ||
                      value.include?("windows") ||
                      value.include?("win32")

  nil
end

def normalize_target_os(value)
  return nil if value.nil?

  case value.downcase
  when "freebsd", "linux", "darwin", "windows", "android"
    value.downcase
  else
    nil
  end
end

def infer_arch(target)
  arch = target.to_s.split("-").first.to_s.downcase

  case arch
  when "amd64"
    "x86_64"
  when "arm64"
    "aarch64"
  else
    arch
  end
end

def os_version(target, target_os)
  pattern = /#{Regexp.escape(target_os)}([0-9][0-9.]*)?/
  match = target.to_s.downcase.match(pattern)
  return nil unless match

  match[1]
end

def target_abi(target, target_os)
  value = target.to_s.downcase

  case target_os
  when "linux"
    return "musl"      if value.include?("musl")
    return "uclibc"    if value.include?("uclibc")
    return "gnueabihf" if value.include?("gnueabihf")
    return "gnueabi"   if value.include?("gnueabi")
    return "gnu"       if value.include?("gnu")

    # Linux triples that omit a libc token usually target glibc.
    "gnu"
  when "windows"
    return "msvc"  if value.include?("msvc")
    return "mingw" if value.include?("mingw") ||
                      value.include?("w64") ||
                      value.include?("gnu")

    "windows"
  else
    "native"
  end
end

def native_target?(host_target, target, host_os, target_os, host_arch,
                   target_arch)
  return false unless host_os == target_os
  return false unless host_arch == target_arch

  case target_os
  when "freebsd", "darwin"
    host_version = os_version(host_target, host_os)
    target_version = os_version(target, target_os)

    target_version.nil? || host_version == target_version
  else
    target_abi(host_target, host_os) == target_abi(target, target_os)
  end
end

def sh_capture (*cmd)
  output = `#{Shellwords.join(cmd)}`

  unless $CHILD_STATUS.success?
    abort "command failed: #{cmd.join(" ")}"
  end

  output.strip
end

def pkg_config_exists?(*packages)
  system("pkg-config", "--exists", *packages)
end

def pkg_config_cflags(*packages)
  sh_capture("pkg-config",
              "--cflags",
              *packages).split
end

def external_header_cflags(flags)
  flags.flat_map do |flag|
    if flag.start_with?("-I") && flag.length > 2
      [
        "-isystem",
        flag.delete_prefix("-I")
      ]
    else
      [flag]
    end
  end
end

def pkg_config_libs(*packages)
  sh_capture("pkg-config",
             "--libs",
             *packages).split
end

def sh_cmd(*cmd)
  sh(*cmd)
end

def dest_path(path)
  File.join(BUILD[:destdir], path)
end

def gtk3_module_path
  dest_path(File.join(BUILD[:gtk3_im_moduledir],
                      "im-cim-gtk3.so"))
end

def qt6_module_path
  dest_path(File.join(BUILD[:qt6_im_moduledir],
                      "libqt6im-cim.so"))
end

def first_executable(*paths)
  paths.find { |path| File.executable?(path) }
end

def first_directory(*patterns)
  patterns.flat_map { |pattern| Dir.glob(pattern) }
          .find { |path| File.directory?(path) }
end

BUILD = {}

SUPPORTED_TARGET_OS = %w[freebsd linux darwin windows android].freeze
SUPPORTED_PROFILES  = %w[debug release].freeze

HOST_CC = env_choice("CIM_HOST_CC", nil,
                     find_program("cc") || find_program("gcc") || "cc")
HOST_CC_WORDS = command_words(HOST_CC)
HOST_TARGET = env_choice(
  "CIM_HOST_TARGET",
  nil,
  capture_command(*(HOST_CC_WORDS + ["-dumpmachine"]))
)
HOST_OS = infer_target_os(HOST_TARGET) ||
          infer_target_os(RbConfig::CONFIG.fetch("host_os", ""))
HOST_ARCH = infer_arch(HOST_TARGET)

fail_config "cannot infer host OS from #{HOST_TARGET.inspect}" unless HOST_OS

HOST_ABI = target_abi(HOST_TARGET, HOST_OS)

TARGET = env_choice("CIM_TARGET", "TARGET", HOST_TARGET)
TARGET_ARCH = infer_arch(TARGET)
INFERRED_TARGET_OS = infer_target_os(TARGET)
EXPLICIT_TARGET_OS = normalize_target_os(env_choice("CIM_TARGET_OS", "OS"))

if env_choice("CIM_TARGET_OS", "OS") && EXPLICIT_TARGET_OS.nil?
  fail_config "unsupported target OS: #{env_choice('CIM_TARGET_OS', 'OS')}"
end

if EXPLICIT_TARGET_OS && INFERRED_TARGET_OS &&
   EXPLICIT_TARGET_OS != INFERRED_TARGET_OS
  fail_config "CIM_TARGET_OS=#{EXPLICIT_TARGET_OS} conflicts with " \
              "CIM_TARGET=#{TARGET}"
end

TARGET_OS = EXPLICIT_TARGET_OS || INFERRED_TARGET_OS
fail_config "cannot infer target OS from #{TARGET.inspect}; set CIM_TARGET_OS" \
  unless TARGET_OS
fail_config "unsupported target OS: #{TARGET_OS}" \
  unless SUPPORTED_TARGET_OS.include?(TARGET_OS)
TARGET_ABI = target_abi(TARGET, TARGET_OS)

profile_value = env_choice("CIM_BUILD_PROFILE", "PROFILE")
if profile_value && ENV["BUILD"] && profile_value != ENV["BUILD"]
  fail_config "CIM_BUILD_PROFILE/PROFILE and BUILD disagree"
end

BUILD_PROFILE = profile_value || ENV["BUILD"] || "release"
fail_config "unsupported build profile: #{BUILD_PROFILE}" \
  unless SUPPORTED_PROFILES.include?(BUILD_PROFILE)

NATIVE_TARGET = native_target?(HOST_TARGET, TARGET, HOST_OS, TARGET_OS,
                               HOST_ARCH, TARGET_ARCH)
GPR_TARGET = env_choice(
  "CIM_GPR_TARGET",
  "GPR_TARGET",
  NATIVE_TARGET ? nil : TARGET
)

BUILD[:cc] =
  ENV["CC"] || "cc"

BUILD[:cxx] =
  ENV["CXX"] || "c++"

BUILD[:rake] =
  ENV["RAKE"] || "rake"

BUILD[:gprbuild] =
  ENV["GPRBUILD"] || "gprbuild"

BUILD[:gprclean] =
  ENV["GPRCLEAN"] || "gprclean"

BUILD[:prefix] =
  ENV["PREFIX"] || "/usr/local"

BUILD[:destdir] =
  ENV["DESTDIR"] || ""

BUILD[:libdir] =
  ENV["LIBDIR"] || File.join(BUILD[:prefix], "lib")

BUILD[:datadir] =
  File.join(BUILD[:prefix], "share")

BUILD[:locale_dir] =
  File.join(BUILD[:datadir], "locale")

BUILD[:input_d_dir] =
  File.join(BUILD[:libdir], "input.d")

BUILD[:dl_ldflag] =
  TARGET_OS == "linux" ? "-ldl" : nil

BUILD[:cflags] =
  %w[
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
    -Wpointer-arith
    -Wcast-qual
    -Wformat=2
    -Wundef
    -Wstrict-prototypes
    -fPIC
    -std=c23
  ]

if NATIVE_TARGET && pkg_config_exists?("gtk+-3.0")
  BUILD[:gtk3_cflags] =
    pkg_config_cflags("gtk+-3.0")

  BUILD[:gtk3_libs] =
    pkg_config_libs("gtk+-3.0")

  BUILD[:gtk3_libdir] =
    sh_capture("pkg-config",
               "--variable=libdir",
               "gtk+-3.0")

  BUILD[:gtk3_binary_version] =
    sh_capture("pkg-config",
               "--variable=gtk_binary_version",
               "gtk+-3.0")

  bit =
    case `uname -m`.strip
    when "x86_64", "amd64", "aarch64"
      64
    else
      32
    end

  gtk_query_path =
    "PATH=$PATH:#{BUILD[:gtk3_libdir]}/libgtk-3-0"

  BUILD[:gtk_query_immodules3] =
    `#{gtk_query_path} which gtk-query-immodules-3.0 2>/dev/null`.strip

  if BUILD[:gtk_query_immodules3].empty?
    BUILD[:gtk_query_immodules3] =
      `#{gtk_query_path} which gtk-query-immodules-3.0-#{bit} 2>/dev/null`.strip
  end

  if BUILD[:gtk_query_immodules3].empty?
    BUILD[:gtk_query_immodules3] = nil
  end

  BUILD[:gtk3_im_moduledir] =
    File.join(BUILD[:gtk3_libdir],
              "gtk-3.0",
              BUILD[:gtk3_binary_version],
              "immodules")

  BUILD[:gtk3_enabled] =
    !BUILD[:gtk_query_immodules3].nil?
else
  BUILD[:gtk3_enabled] =
    false
end

if NATIVE_TARGET &&
   pkg_config_exists?("Qt6Core", "Qt6Gui", "Qt6Widgets")
  BUILD[:qt6_cflags] =
    pkg_config_cflags("Qt6Core", "Qt6Gui", "Qt6Widgets")

  BUILD[:qt6_libs] =
    pkg_config_libs("Qt6Core", "Qt6Gui", "Qt6Widgets")

  BUILD[:qt6_moc] =
    ENV["MOC_QT6"] ||
    first_executable(
      "/usr/local/libexec/qt6/moc",     # FreeBSD
      "/usr/lib/qt6/moc",               # Manjaro
      "/usr/lib64/qt6/libexec/moc",     # openSUSE Tumbleweed
      "/usr/lib/qt6/libexec/moc"        # Debian Bookworm
    )

  BUILD[:qt6_core_private_include_path] =
    ENV["QT6_CORE_PRIVATE_INCLUDE_PATH"] ||
    first_directory(
      "/usr/include/qt6/QtCore/6.*.*",
      "/usr/include/x86_64-linux-gnu/qt6/QtCore/6.*.*", # Debian Bookworm
      "/usr/local/include/qt6/QtCore/6.*.*"             # FreeBSD
    )

  BUILD[:qt6_gui_private_include_path] =
    ENV["QT6_GUI_PRIVATE_INCLUDE_PATH"] ||
    first_directory(
      "/usr/include/qt6/QtGui/6.*.*",
      "/usr/include/x86_64-linux-gnu/qt6/QtGui/6.*.*", # Debian Bookworm
      "/usr/local/include/qt6/QtGui/6.*.*"             # FreeBSD
    )

  BUILD[:qt6_im_moduledir] =
    ENV["QT6_IM_MODULE_DIR"] ||
    first_directory(
      "/usr/lib/qt6/plugins/platforminputcontexts",
      "/usr/local/lib/qt6/plugins/platforminputcontexts",              # FreeBSD
      "/usr/lib64/qt6/plugins/platforminputcontexts",                  # openSUSE Tumbleweed
      "/usr/lib/x86_64-linux-gnu/qt6/plugins/platforminputcontexts"    # Debian Bookworm
    )

  BUILD[:qt6_enabled] =
    !BUILD[:qt6_moc].nil? &&
    !BUILD[:qt6_core_private_include_path].nil? &&
    !BUILD[:qt6_gui_private_include_path].nil? &&
    !BUILD[:qt6_im_moduledir].nil?
else
  BUILD[:qt6_enabled] =
    false
end

# Bypasses Rake's default task resolution for undefined arguments,
# enabling positional library types (e.g., `rake test relocatable`).
ARGV.drop(1).each do |arg|
  task arg.to_sym do; end unless Rake::Task.task_defined?(arg)
end

SUPPORTED_LIBRARY_TYPES = %w[static-pic relocatable].freeze

# Resolve an optional positional library type such as `rake test relocatable`.
positional_lib_types =
  ARGV & (SUPPORTED_LIBRARY_TYPES + ["static"])

if positional_lib_types.include?("static")
  fail_config "non-PIC static builds are unsupported; use static-pic"
end

if positional_lib_types.length > 1
  fail_config "multiple positional library types were specified"
end

positional_lib_type = positional_lib_types.first
env_lib_type = env_choice("CIM_LIBRARY_TYPE", "LIBRARY_TYPE")

if env_lib_type == "static"
  fail_config "non-PIC static builds are unsupported; use static-pic"
end

if env_lib_type && positional_lib_type &&
   env_lib_type != positional_lib_type
  fail_config "CIM_LIBRARY_TYPE/LIBRARY_TYPE and positional library type disagree"
end

LIB_TYPE = env_lib_type || positional_lib_type || "static-pic"
fail_config "unsupported library type: #{LIB_TYPE}" \
  unless SUPPORTED_LIBRARY_TYPES.include?(LIB_TYPE)

RAKE     = BUILD[:rake]
CC       = BUILD[:cc]
CXX      = BUILD[:cxx]
GPRBUILD = BUILD[:gprbuild]
GPRCLEAN = BUILD[:gprclean]

BUILD_ROOT = "build"
OBJ_PROFILE_DIR =
  File.join(BUILD_ROOT, "obj", TARGET, BUILD_PROFILE)
OBJ_DIR = File.join(OBJ_PROFILE_DIR, LIB_TYPE)
LIB_DIR = File.join(BUILD_ROOT, "lib", TARGET, BUILD_PROFILE)
BIN_DIR = File.join(BUILD_ROOT, "bin", TARGET, BUILD_PROFILE)

LIBCIM_FILE =
  File.join(LIB_DIR, LIB_TYPE == "relocatable" ? "libcim.so" : "libcim.a")
CIM_LINK_LIBS =
  LIB_TYPE == "relocatable" ? ["-L#{LIB_DIR}", "-lcim"] : [LIBCIM_FILE]

CLAIR_DIR = File.expand_path("../ada-clair", SRC_TOP)
CLAIR_INCLUDE_DIR = File.join(CLAIR_DIR, "include")
CLAIR_HEADER = File.join(CLAIR_INCLUDE_DIR, "clair.h")
CLAIR_LIB_DIR = File.join(CLAIR_DIR, "build", "lib", TARGET, BUILD_PROFILE)
CLAIR_LIB_FILE =
  File.join(
    CLAIR_LIB_DIR,
    LIB_TYPE == "relocatable" ? "libclair.so" : "libclair.a"
  )
CLAIR_LINK_LIBS =
  LIB_TYPE == "relocatable" ?
    ["-L#{CLAIR_LIB_DIR}", "-lclair"] :
    [CLAIR_LIB_FILE]
CLAIR_GEN_DIR = File.join(CLAIR_DIR, "build", "gen", TARGET)
CLAIR_PROBE_FILES = %w[
  .probe-stamp
  clair-errno.ads
  clair-platform.ads
].map { |name| File.join(CLAIR_GEN_DIR, name) }.freeze

# gnatls -v 명령의 결과에서 실제 에이다 오브젝트 표준 경로(adalib)를 정밀하게 파싱합니다.
GNAT_LIB_DIR = `gnatls -v`.lines.find { |line| line.include?("adalib") }&.strip || ""
# libgnarl depends on symbols provided by libgnat, so keep libgnat last for
# left-to-right static archive resolution.
ADA_STATIC_LIBS = [
  File.join(GNAT_LIB_DIR, "libgnarl_pic.a"),
  File.join(GNAT_LIB_DIR, "libgnat_pic.a")
]

# 동적(.so) 모듈에 넣을 수 있도록 PIC 에이다 런타임을 사용합니다.
ADA_LIBS = (LIB_TYPE == "relocatable" ? [] : ADA_STATIC_LIBS)

CLAIR_SYSTEM_LIBS =
  case TARGET_OS
  when "freebsd"
    %w[-L/usr/local/lib -lintl -lpthread -lpcre2-8 -lyaml]
  when "darwin"
    %w[-lpcre2-8 -lyaml]
  when "linux"
    %w[-ldl -lpcre2-8 -lyaml]
  when "windows"
    dependency_root =
      File.join(CLAIR_DIR, "build", "deps", TARGET, BUILD_PROFILE)
    libyaml_prefix =
      ENV["CLAIR_LIBYAML_PREFIX"] ||
      File.join(dependency_root, "libyaml")
    pcre2_prefix =
      ENV["CLAIR_PCRE2_PREFIX"] ||
      File.join(dependency_root, "pcre2")
    [
      "-L#{pcre2_prefix}/lib",
      "-L#{libyaml_prefix}/lib",
      "-lpcre2-8",
      "-lyaml"
    ]
  when "android"
    %w[-ldl -lyaml]
  end

CLAIR_LINK_CLOSURE =
  LIB_TYPE == "relocatable" ?
    CLAIR_LINK_LIBS :
    CLAIR_LINK_LIBS + ADA_LIBS + CLAIR_SYSTEM_LIBS

SAMPLE_THREAD_LIBS =
  TARGET_OS == "freebsd" ? %w[-lstdthreads -lpthread] : %w[-lpthread]

DYNAMIC_LOADER_LIBS = [BUILD[:dl_ldflag]].compact.freeze

NESTED_HOST_TEST_ENABLED =
  LIB_TYPE == "static-pic" && %w[freebsd linux].include?(TARGET_OS)

PLUGIN_REENTRY_TEST_ENABLED =
  %w[freebsd linux].include?(TARGET_OS)

TEST_CFLAGS = %w[
  -Wall
  -Wextra
  -Werror
  -std=c23
  -D_POSIX_C_SOURCE=200809L
  -Iinclude
] +
  (LIB_TYPE == "relocatable" ? [] :
    %w[-DCIM_HAS_INTERNAL_TEST_HOOK=1]) +
  (PLUGIN_REENTRY_TEST_ENABLED ?
    %w[-DCIM_HAS_PLUGIN_REENTRY_TEST=1] : [])

SAMPLE_CFLAGS = %w[
  -Wall
  -Wextra
  -Werror
  -Wpedantic
  -Wconversion
  -Wshadow
  -Wpointer-arith
  -Wcast-qual
  -Wformat=2
  -Wundef
  -Wstrict-prototypes
  -fPIC
  -std=c23
  -Iinclude
]

SAMPLE_LIBHANGUL_CFLAGS =
  external_header_cflags(
    ENV["LIBHANGUL_CFLAGS"]&.split ||
    (NATIVE_TARGET ? `pkg-config --cflags libhangul 2>/dev/null`.split : [])
  )

SAMPLE_LIBHANGUL_LIBS =
  ENV["LIBHANGUL_LIBS"]&.split ||
  (NATIVE_TARGET ? `pkg-config --libs libhangul 2>/dev/null`.split : [])

TEST_PLUGIN_SOURCES = {
  "bridge-callback" => "tests/plugins/im-bridge-callback.c",
  "dummy"       => "tests/plugins/im-dummy.c",
  "init-fail"   => "tests/plugins/im-init-fail.c",
  "create-fail" => "tests/plugins/im-create-fail.c",
  "bad-version" => "tests/plugins/im-bad-version.c",
  "no-symbol"   => "tests/plugins/im-no-symbol.c",
  "no-create"    => "tests/plugins/im-no-create.c",
  "no-destroy"   => "tests/plugins/im-no-destroy.c",
  "noop-destroy" => "tests/plugins/im-noop-destroy.c",
  "nested-counter" => "tests/plugins/im-nested-counter.c"
}

if PLUGIN_REENTRY_TEST_ENABLED
  TEST_PLUGIN_SOURCES["reentrant-init"] =
    "tests/plugins/im-reentrant-init.c"
  TEST_PLUGIN_SOURCES["reentrant-fini"] =
    "tests/plugins/im-reentrant-fini.c"
end

TEST_PLUGIN_TARGETS =
  TEST_PLUGIN_SOURCES.keys.map do |name|
    File.join(LIB_DIR, "im-#{name}.so")
  end

def gpr_target_args
  GPR_TARGET.to_s.empty? ? [] : ["--target=#{GPR_TARGET}"]
end

def gpr_external_args(library_type = LIB_TYPE)
  [
    "-XCIM_TARGET=#{TARGET}",
    "-XCIM_TARGET_OS=#{TARGET_OS}",
    "-XCIM_BUILD_PROFILE=#{BUILD_PROFILE}",
    "-XCIM_LIBRARY_TYPE=#{library_type}",
    "-XCLAIR_TARGET=#{TARGET}",
    "-XCLAIR_TARGET_OS=#{TARGET_OS}",
    "-XCLAIR_BUILD_PROFILE=#{BUILD_PROFILE}",
    "-XCLAIR_LIBRARY_TYPE=#{library_type}"
  ]
end

def gprbuild_project(project, library_type = LIB_TYPE)
  object_dir = File.join(OBJ_PROFILE_DIR, library_type)
  mkdir_p [object_dir, LIB_DIR, BIN_DIR]
  sh_cmd(*(command_words(GPRBUILD) + gpr_target_args +
           gpr_external_args(library_type) +
           ["-P", project]))
end

def gprclean_project(project)
  return unless command_available?(command_words(GPRCLEAN).first)
  return unless NATIVE_TARGET
  return unless File.directory?(OBJ_DIR)

  sh_cmd(*(command_words(GPRCLEAN) + gpr_target_args + gpr_external_args +
           ["-r", "-P", project]))
end

def ensure_clair_probe_current
  return if CLAIR_PROBE_FILES.all? { |path| File.file?(path) }

  if NATIVE_TARGET
    Dir.chdir(CLAIR_DIR) do
      sh_cmd(RAKE,
             "probe",
             "TARGET=#{TARGET}",
             "CLAIR_BUILD_PROFILE=#{BUILD_PROFILE}")
    end
    return if CLAIR_PROBE_FILES.all? { |path| File.file?(path) }
  end

  fail_config <<~MSG
    ada-clair probe artifacts are missing for #{TARGET}.
    Expected files under #{CLAIR_GEN_DIR}.
    Cross builds never run target probes implicitly.
  MSG
end

def ensure_native_task(task_name)
  return if NATIVE_TARGET

  fail_config "#{task_name} is native-only for now; " \
              "target=#{TARGET}, host=#{HOST_TARGET}"
end

def lib_rpath_flag
  "-Wl,-rpath,#{File.expand_path(LIB_DIR)}"
end

def clair_rpath_flags
  return [] unless LIB_TYPE == "relocatable"

  ["-Wl,-rpath,#{File.expand_path(CLAIR_LIB_DIR)}"]
end

def embedded_archive_link_flags
  return [] if LIB_TYPE == "relocatable"

  case TARGET_OS
  when "linux", "freebsd", "android"
    %w[-Wl,--exclude-libs,ALL]
  else
    []
  end
end

def test_export_dynamic_flags
  return [] unless PLUGIN_REENTRY_TEST_ENABLED

  %w[-Wl,--export-dynamic]
end

GTK3_IM_OBJ = File.join(OBJ_DIR, "im-cim-gtk3.o")
GTK3_CANDIDATE_OBJ = File.join(OBJ_DIR, "c-candidate.o")
GTK3_CANDIDATE_DATA_OBJ = File.join(OBJ_DIR, "c-candidate-data.o")
GTK3_PREEDIT_OBJ = File.join(OBJ_DIR, "c-preedit.o")
GTK3_MODULE = File.join(LIB_DIR, "im-cim-gtk3.so")

QT6_MOC = File.join(OBJ_DIR, "im-cim-qt.moc")
QT6_OBJ = File.join(OBJ_DIR, "im-cim-qt6.o")
QT6_MODULE = File.join(LIB_DIR, "libqt6im-cim.so")

SAMPLE_LIBHANGUL_MODULE = File.join(LIB_DIR, "im-libhangul.so")

CIM_ADA_TEST_PROJECT = "tests/ada/cim-tests.gpr"
CIM_ADA_TEST_EXE = File.join(BIN_DIR, "cim-unit-tests")

TEST_CIM_EXE = File.join(BIN_DIR, "test-cim")
TEST_LIBHANGUL_EXE = File.join(BIN_DIR, "test-im-libhangul")
TEST_GTK_PREEDIT_EXE = File.join(BIN_DIR, "test-gtk-preedit")
TEST_GTK_CANDIDATE_EXE = File.join(BIN_DIR, "test-gtk-candidate")
TEST_GTK_CALLBACK_EXE = File.join(BIN_DIR, "test-gtk-callback-delivery")
TEST_QT6_CALLBACK_EXE = File.join(BIN_DIR, "test-qt6-callback-delivery")

NESTED_INNER_PLUGIN =
  File.join(LIB_DIR, "libim-nested-counter.so")
NESTED_SHARED_CIM = File.join(LIB_DIR, "libcim.so")
NESTED_SHARED_CLAIR =
  File.join(CLAIR_LIB_DIR, "libclair.so")
NESTED_OUTER_SOURCE = "tests/outer-plugins/outer-cim-host.c"
NESTED_OUTER_MODULES = {
  File.join(LIB_DIR, "outer-cim-a.so") =>
    ["outer_cim_a_open", "outer_cim_a_close"],
  File.join(LIB_DIR, "outer-cim-b.so") =>
    ["outer_cim_b_open", "outer_cim_b_close"]
}.freeze
TEST_NESTED_HOST_EXE = File.join(BIN_DIR, "test-nested-host")

desc "Print the resolved Cim build context"
task :info do
  puts "HOST_TARGET=#{HOST_TARGET}"
  puts "HOST_OS=#{HOST_OS}"
  puts "HOST_ABI=#{HOST_ABI}"
  puts "CIM_TARGET=#{TARGET}"
  puts "CIM_TARGET_OS=#{TARGET_OS}"
  puts "CIM_TARGET_ABI=#{TARGET_ABI}"
  puts "CIM_BUILD_PROFILE=#{BUILD_PROFILE}"
  puts "CIM_LIBRARY_TYPE=#{LIB_TYPE}"
  puts "CIM_GPR_TARGET=#{GPR_TARGET}" unless GPR_TARGET.to_s.empty?
  puts "NATIVE_TARGET=#{NATIVE_TARGET}"
  puts "OBJ_DIR=#{OBJ_DIR}"
  puts "LIB_DIR=#{LIB_DIR}"
  puts "BIN_DIR=#{BIN_DIR}"
end

desc "Show help message with available build options"
task :help do
  puts <<~HELP
    ============================================================================
    CIM Build System Help & Usage Guidelines
    ============================================================================
    Usage:
      rake [task] [type]

    [1] Primary Build Tasks:
      rake info            : Show resolved target/profile/library settings
      rake                 : Build everything for the native target
      rake cim             : Build only libcim core (Ada Core + C Wrapper)
      rake plugins         : Build test plugins inside tests/plugins/
      rake samples         : Build sample modules (modules/samples/im-libhangul)
      rake tests           : Build test executables (bin/test-*)
      rake test            : Build and run the consolidated test suite
      rake clean           : Clean all build artifacts and object directories
      rake rebuild         : Clean and rebuild everything from scratch

    [2] Library Type Options (Positional Arguments):
      static-pic           : (Default) Build position-independent static archive (.a)
                             * For embedding into shared modules
      relocatable          : Build standalone shared library (.so)
                             * Uses encapsulated SAL runtime

    [3] Real-world Examples:
      rake info TARGET=x86_64-unknown-freebsd
      rake cim TARGET=x86_64-unknown-freebsd
      rake cim TARGET=x86_64-linux-gnu PROFILE=debug
      rake cim TARGET=x86_64-linux-musl
      rake cim relocatable TARGET=x86_64-unknown-freebsd
      rake test TARGET=x86_64-unknown-freebsd

    [4] Target/Profile Variables:
      CIM_TARGET           : Canonical toolchain target triple
      CIM_TARGET_OS        : freebsd, linux, darwin, windows, android
      CIM_BUILD_PROFILE    : debug or release
      CIM_LIBRARY_TYPE     : static-pic or relocatable

      Linux libc variants are separate ABI targets. For example,
      x86_64-linux-gnu and x86_64-linux-musl are not native-compatible
      with each other.

      TARGET, OS, PROFILE, BUILD, and LIBRARY_TYPE are accepted as
      compatibility aliases where unambiguous.
    ============================================================================
  HELP
end

desc "Build everything"
task :build do
  ensure_native_task("build")
  %i[cim test-plugins samples bridges tests].each do |task_name|
    Rake::Task[task_name].invoke
  end
end

task :clair do
  sh(
    { "CLAIR_LIBRARY_TYPE" => LIB_TYPE },
    *command_words(RAKE),
    "-C",
    CLAIR_DIR,
    "build",
    "TARGET=#{TARGET}",
    "BUILD=#{BUILD_PROFILE}"
  )
end

desc "Build libcim (Ada Core + C Wrapper)"
file LIBCIM_FILE => [
  :clair,
  "cim.gpr",
  "cim_objects.gpr",
  "cim-elf.map",
  "src/cim-c-api.c",
  "src/cim-internal.c",
  "src/cim.ads",
  "src/cim.adb",
  "src/cim-ic.ads",
  "src/cim-ic.adb",
  "src/cim-runtime.ads",
  "src/cim-runtime.adb",
  "src/cim-c.ads",
  "src/cim-c.adb",
  "include/cim.h"
] do
  ensure_clair_probe_current
  gprbuild_project("cim.gpr")
  abort "Expected library not found: #{LIBCIM_FILE}" unless
    File.exist?(LIBCIM_FILE)
  abort "Expected library not found: #{CLAIR_LIB_FILE}" unless
    File.exist?(CLAIR_LIB_FILE)
end

desc "Build libcim (Ada Core + C Wrapper)"
task :cim => [LIBCIM_FILE]

TEST_PLUGIN_SOURCES.each do |name, source|
  plugin_cflags = name == "bridge-callback" ? %w[-pthread] : []
  soname =
    File.join(LIB_DIR, "libim-#{name}.so")

  linkname =
    File.join(LIB_DIR, "im-#{name}.so")

  file soname => [
    source,
    "include/cim.h"
  ] do
    ensure_native_task("test plugin build")
    mkdir_p LIB_DIR

    sh_cmd(*([CC] +
             BUILD[:cflags] +
             plugin_cflags +
             [
               "-Iinclude",
               "-shared",
               source,
               "-o",
               soname
             ]))
  end

  file linkname => [soname] do
    mkdir_p File.dirname(linkname)
    rm_f linkname
    ln_sf File.basename(soname),
          linkname
  end
end

desc "Build test plugins"
task :"test-plugins" => TEST_PLUGIN_TARGETS
task :plugins => :"test-plugins"

task :nested_shared_cim => [LIBCIM_FILE] do
  unless NESTED_HOST_TEST_ENABLED
    abort "nested shared Cim fixture requires static-pic ELF embedding"
  end

  ensure_native_task("nested shared Cim fixture build")

  sh(
    {
      "CLAIR_LIBRARY_TYPE" => "relocatable"
    },
    *command_words(RAKE),
    "-C",
    CLAIR_DIR,
    "build",
    "TARGET=#{TARGET}",
    "BUILD=#{BUILD_PROFILE}"
  )

  gprbuild_project("cim.gpr", "relocatable")

  abort "Expected nested shared Cim not found: #{NESTED_SHARED_CIM}" unless
    File.exist?(NESTED_SHARED_CIM)
  abort "Expected nested shared Clair not found: #{NESTED_SHARED_CLAIR}" unless
    File.exist?(NESTED_SHARED_CLAIR)
end

NESTED_OUTER_MODULES.each do |module_path, symbols|
  open_symbol, close_symbol = symbols

  file module_path => [
    LIBCIM_FILE,
    NESTED_OUTER_SOURCE,
    "include/cim.h"
  ] do
    unless NESTED_HOST_TEST_ENABLED
      abort "nested host tests require static ELF embedding"
    end

    ensure_native_task("nested host module build")
    mkdir_p LIB_DIR

    sh_cmd(*([CC] +
             BUILD[:cflags] +
             [
               "-fvisibility=hidden",
               "-Iinclude",
               "-DCIM_OUTER_OPEN=#{open_symbol}",
               "-DCIM_OUTER_CLOSE=#{close_symbol}",
               "-shared",
               NESTED_OUTER_SOURCE
             ] +
             embedded_archive_link_flags +
             CIM_LINK_LIBS +
             CLAIR_LINK_CLOSURE +
             [
               "-lpthread",
               "-o",
               module_path
             ]))
  end
end

desc "Build sample modules"
task :samples => [:sample_libhangul]

bridge_tasks = []
bridge_tasks << :gtk3 if BUILD[:gtk3_enabled]
bridge_tasks << :qt6 if BUILD[:qt6_enabled]

desc "Build bridge modules"
task :bridges => bridge_tasks

file GTK3_IM_OBJ => [
  "modules/bridges/gtk3/im-cim-gtk.c",
  "modules/bridges/gtk3/c-candidate.h",
  "modules/bridges/gtk3/c-candidate-data.h",
  "modules/bridges/gtk3/c-preedit.h",
  "include/cim.h"
] do
  unless BUILD[:gtk3_enabled]
    abort "GTK3 bridge cannot be built: gtk+-3.0 or gtk-query-immodules-3.0 not found"
  end

  mkdir_p OBJ_DIR

  gtk3_cflags =
    BUILD[:cflags] +
    BUILD[:gtk3_cflags] +
    [
      "-fvisibility=hidden",
      "-Iinclude",
      "-DCIM_LOCALE_DIR=#{BUILD[:locale_dir].dump}"
    ]

  sh_cmd(*([CC] +
           gtk3_cflags +
             [
               "-c",
               "modules/bridges/gtk3/im-cim-gtk.c",
               "-o",
               GTK3_IM_OBJ
             ]))
end

file GTK3_CANDIDATE_OBJ => [
  "modules/bridges/gtk3/c-candidate.c",
  "modules/bridges/gtk3/c-candidate.h",
  "modules/bridges/gtk3/c-candidate-data.h",
  "include/cim.h"
] do
  mkdir_p OBJ_DIR

  gtk3_cflags =
    BUILD[:cflags] +
    BUILD[:gtk3_cflags] +
    [
      "-fvisibility=hidden",
      "-Iinclude",
      "-DCIM_LOCALE_DIR=#{BUILD[:locale_dir].dump}"
    ]

  sh_cmd(*([CC] +
           gtk3_cflags +
             [
               "-c",
               "modules/bridges/gtk3/c-candidate.c",
               "-o",
               GTK3_CANDIDATE_OBJ
             ]))
end

file GTK3_CANDIDATE_DATA_OBJ => [
  "modules/bridges/gtk3/c-candidate-data.c",
  "modules/bridges/gtk3/c-candidate-data.h",
  "include/cim.h"
] do
  mkdir_p OBJ_DIR

  gtk3_cflags =
    BUILD[:cflags] +
    BUILD[:gtk3_cflags] +
    [
      "-fvisibility=hidden",
      "-Iinclude",
      "-DCIM_LOCALE_DIR=#{BUILD[:locale_dir].dump}"
    ]

  sh_cmd(*([CC] +
           gtk3_cflags +
             [
               "-c",
               "modules/bridges/gtk3/c-candidate-data.c",
               "-o",
               GTK3_CANDIDATE_DATA_OBJ
             ]))
end

file GTK3_PREEDIT_OBJ => [
  "modules/bridges/gtk3/c-preedit.c",
  "modules/bridges/gtk3/c-preedit.h",
  "include/cim.h"
] do
  mkdir_p OBJ_DIR

  gtk3_cflags =
    BUILD[:cflags] +
    BUILD[:gtk3_cflags] +
    [
      "-fvisibility=hidden",
      "-Iinclude",
      "-DCIM_LOCALE_DIR=#{BUILD[:locale_dir].dump}"
    ]

  sh_cmd(*([CC] +
           gtk3_cflags +
             [
               "-c",
               "modules/bridges/gtk3/c-preedit.c",
               "-o",
               GTK3_PREEDIT_OBJ
             ]))
end

file GTK3_MODULE => [
  LIBCIM_FILE,
  GTK3_IM_OBJ,
  GTK3_CANDIDATE_OBJ,
  GTK3_CANDIDATE_DATA_OBJ,
  GTK3_PREEDIT_OBJ
] do
  mkdir_p LIB_DIR

  gtk3_libs =
    BUILD[:gtk3_libs] +
    embedded_archive_link_flags +
    CIM_LINK_LIBS +
    CLAIR_LINK_CLOSURE +
    clair_rpath_flags +
    [
      "-Wl,-rpath,$ORIGIN"
    ]

  sh_cmd(*([CC] +
           %w[-shared -fPIC] +
           [
             GTK3_IM_OBJ,
             GTK3_CANDIDATE_OBJ,
             GTK3_CANDIDATE_DATA_OBJ,
             GTK3_PREEDIT_OBJ
           ] +
           gtk3_libs +
           [
             "-Wl,-soname",
             "-Wl,im-cim-gtk3.so",
             "-o",
             GTK3_MODULE
           ]))
end

desc "Build modules/bridges/gtk3"
task :gtk3 => [GTK3_MODULE]

file QT6_MOC => [
  "modules/bridges/qt6/im-cim-qt.cpp",
  CLAIR_HEADER
] do
  unless BUILD[:qt6_enabled]
    abort "Qt6 bridge cannot be built: Qt6Core, Qt6Gui, Qt6Widgets, or moc not found"
  end

  mkdir_p OBJ_DIR

  sh_cmd(BUILD[:qt6_moc],
         "-I",
         CLAIR_INCLUDE_DIR,
         "-I",
         CLAIR_GEN_DIR,
         "-I",
         BUILD[:qt6_core_private_include_path],
         "-I",
         BUILD[:qt6_gui_private_include_path],
         "modules/bridges/qt6/im-cim-qt.cpp",
         "-o",
         QT6_MOC)
end

file QT6_OBJ => [
  "modules/bridges/qt6/im-cim-qt.cpp",
  "modules/bridges/qt6/cim.json",
  QT6_MOC,
  "include/cim.h",
  CLAIR_HEADER
] do
  mkdir_p OBJ_DIR

  qt6_cxxflags =
    BUILD[:qt6_cflags] +
    [
      "-Iinclude",
      "-I#{CLAIR_INCLUDE_DIR}",
      "-I#{CLAIR_GEN_DIR}",
      "-I#{OBJ_DIR}",
      "-I#{BUILD[:qt6_core_private_include_path]}",
      "-I#{BUILD[:qt6_gui_private_include_path]}",
      "-DQT_NO_KEYWORDS",
      "-fvisibility=hidden",
      "-std=c++17"
    ]

  sh_cmd(*([CXX] +
           %w[-fPIC] +
           qt6_cxxflags +
           [
             "-c",
             "modules/bridges/qt6/im-cim-qt.cpp",
             "-o",
             QT6_OBJ
           ]))
end

file QT6_MODULE => [
  LIBCIM_FILE,
  QT6_OBJ
] do
  mkdir_p LIB_DIR

  qt6_libs =
    BUILD[:qt6_libs] +
    embedded_archive_link_flags +
    CIM_LINK_LIBS +
    CLAIR_LINK_CLOSURE +
    clair_rpath_flags +
    [
      "-Wl,-rpath,$ORIGIN"
    ]

  sh_cmd(*([CXX] +
           %w[-shared -fPIC] +
           [
             QT6_OBJ
           ] +
           qt6_libs +
           [
             "-Wl,-soname",
             "-Wl,libqt6im-cim.so",
             "-o",
             QT6_MODULE
           ]))
end

desc "Build modules/bridges/qt6"
task :qt6 => [QT6_MODULE]

desc "Build modules/samples/im-libhangul"
file SAMPLE_LIBHANGUL_MODULE => [
  :clair,
  "modules/samples/im-libhangul/im-libhangul.c",
  "include/cim.h",
  CLAIR_HEADER
] do
  ensure_native_task("sample module build")
  mkdir_p LIB_DIR
  sh_cmd(*([CC] +
          SAMPLE_CFLAGS +
          [
            "-I#{CLAIR_INCLUDE_DIR}",
            "-I#{CLAIR_GEN_DIR}"
          ] +
          SAMPLE_LIBHANGUL_CFLAGS +
          ["modules/samples/im-libhangul/im-libhangul.c"] +
          %w[-shared] +
          SAMPLE_LIBHANGUL_LIBS +
          embedded_archive_link_flags +
          CLAIR_LINK_CLOSURE +
          SAMPLE_THREAD_LIBS +
          clair_rpath_flags +
          [
            "-o", SAMPLE_LIBHANGUL_MODULE
          ]))
end

desc "Build modules/samples/im-libhangul"
task :sample_libhangul => [SAMPLE_LIBHANGUL_MODULE]

file CIM_ADA_TEST_EXE => [
  LIBCIM_FILE,
  CIM_ADA_TEST_PROJECT,
  "tests/ada/cim_unit_tests.adb",
  "tests/ada/cim_process_tests.ads",
  "tests/ada/cim_process_tests.adb",
  "tests/ada/cim_runtime_tests.ads",
  "tests/ada/cim_runtime_tests.adb",
  "tests/ada/cim_tests.ads",
  "tests/ada/cim_tests.adb"
] do
  ensure_native_task("Ada test executable build")
  gprbuild_project(CIM_ADA_TEST_PROJECT)
  abort "Expected test executable not found: #{CIM_ADA_TEST_EXE}" unless
    File.exist?(CIM_ADA_TEST_EXE)
end

file TEST_CIM_EXE => [
  LIBCIM_FILE,
  "tests/test-common.c",
  "tests/test-common.h",
  "tests/test-cim.c",
  "include/cim.h"
] do
  ensure_native_task("test executable build")
  mkdir_p BIN_DIR

  sh_cmd(*([CC] +
           TEST_CFLAGS +
           [
             "tests/test-common.c",
             "tests/test-cim.c"
           ] +
           test_export_dynamic_flags +
           CIM_LINK_LIBS +
           CLAIR_LINK_CLOSURE +
           clair_rpath_flags + [
             "-lpthread",
             lib_rpath_flag,
             "-o",
             TEST_CIM_EXE
           ]))
end

file TEST_LIBHANGUL_EXE => [
  LIBCIM_FILE,
  SAMPLE_LIBHANGUL_MODULE,
  CLAIR_HEADER,
  "tests/test-common.c",
  "tests/test-common.h",
  "tests/test-im-libhangul.c",
  "include/cim.h"
] do
  ensure_native_task("test executable build")
  mkdir_p BIN_DIR

  sh_cmd(*([CC] +
           TEST_CFLAGS +
           [
             "-I#{CLAIR_INCLUDE_DIR}",
             "-I#{CLAIR_GEN_DIR}",
             "tests/test-common.c",
             "tests/test-im-libhangul.c"
            ] + CIM_LINK_LIBS + CLAIR_LINK_CLOSURE + clair_rpath_flags + [
             "-lpthread",
             lib_rpath_flag,
             "-o",
             TEST_LIBHANGUL_EXE
           ]))
end

file TEST_GTK_PREEDIT_EXE => [
  GTK3_PREEDIT_OBJ,
  "tests/test-gtk-preedit.c",
  "modules/bridges/gtk3/c-preedit.h",
  "include/cim.h"
] do
  unless BUILD[:gtk3_enabled]
    abort "GTK3 preedit test cannot be built: GTK3 bridge is disabled"
  end

  ensure_native_task("GTK3 preedit test executable build")
  mkdir_p BIN_DIR

  gtk3_test_cflags =
    TEST_CFLAGS +
    external_header_cflags(BUILD[:gtk3_cflags]) +
    ["-Imodules/bridges/gtk3"]

  sh_cmd(*([CC] +
           gtk3_test_cflags +
           [
             "tests/test-gtk-preedit.c",
             GTK3_PREEDIT_OBJ
           ] +
           BUILD[:gtk3_libs] +
           [
             "-o",
             TEST_GTK_PREEDIT_EXE
           ]))
end

file TEST_GTK_CANDIDATE_EXE => [
  GTK3_CANDIDATE_DATA_OBJ,
  "tests/test-gtk-candidate.c",
  "modules/bridges/gtk3/c-candidate-data.h",
  "include/cim.h"
] do
  unless BUILD[:gtk3_enabled]
    abort "GTK3 candidate test cannot be built: GTK3 bridge is disabled"
  end

  ensure_native_task("GTK3 candidate test executable build")
  mkdir_p BIN_DIR

  gtk3_test_cflags =
    TEST_CFLAGS +
    external_header_cflags(BUILD[:gtk3_cflags]) +
    ["-Imodules/bridges/gtk3"]

  sh_cmd(*([CC] +
           gtk3_test_cflags +
           [
             "tests/test-gtk-candidate.c",
             GTK3_CANDIDATE_DATA_OBJ
           ] +
           BUILD[:gtk3_libs] +
           [
             "-o",
             TEST_GTK_CANDIDATE_EXE
           ]))
end

file TEST_GTK_CALLBACK_EXE => [
  LIBCIM_FILE,
  GTK3_CANDIDATE_OBJ,
  GTK3_CANDIDATE_DATA_OBJ,
  GTK3_PREEDIT_OBJ,
  "tests/test-gtk-callback-delivery.c",
  "modules/bridges/gtk3/im-cim-gtk.c",
  "include/cim.h"
] do
  unless BUILD[:gtk3_enabled]
    abort "GTK3 callback test cannot be built: GTK3 bridge is disabled"
  end

  ensure_native_task("GTK3 callback test executable build")
  mkdir_p BIN_DIR

  gtk3_test_cflags =
    TEST_CFLAGS +
    external_header_cflags(BUILD[:gtk3_cflags]) +
    [
      "-DCIM_BRIDGE_TEST=1",
      "-DCIM_LOCALE_DIR=#{BUILD[:locale_dir].dump}"
    ]

  sh_cmd(*([CC] +
           gtk3_test_cflags +
           [
             "tests/test-gtk-callback-delivery.c",
             "modules/bridges/gtk3/im-cim-gtk.c",
             GTK3_CANDIDATE_OBJ,
             GTK3_CANDIDATE_DATA_OBJ,
             GTK3_PREEDIT_OBJ
           ] +
           BUILD[:gtk3_libs] +
           embedded_archive_link_flags +
           CIM_LINK_LIBS +
           CLAIR_LINK_CLOSURE +
           clair_rpath_flags +
           [
             "-lpthread",
             lib_rpath_flag,
             "-o",
             TEST_GTK_CALLBACK_EXE
           ]))
end

file TEST_QT6_CALLBACK_EXE => [
  LIBCIM_FILE,
  QT6_MOC,
  "tests/test-qt6-callback-delivery.cpp",
  "modules/bridges/qt6/im-cim-qt.cpp",
  "include/cim.h",
  CLAIR_HEADER
] do
  unless BUILD[:qt6_enabled]
    abort "Qt6 callback test cannot be built: Qt6 bridge is disabled"
  end

  ensure_native_task("Qt6 callback test executable build")
  mkdir_p BIN_DIR

  qt6_test_cxxflags =
    BUILD[:qt6_cflags] +
    [
      "-Iinclude",
      "-I#{CLAIR_INCLUDE_DIR}",
      "-I#{CLAIR_GEN_DIR}",
      "-I#{OBJ_DIR}",
      "-I#{BUILD[:qt6_core_private_include_path]}",
      "-I#{BUILD[:qt6_gui_private_include_path]}",
      "-DQT_NO_KEYWORDS",
      "-DCIM_BRIDGE_TEST=1",
      "-std=c++17"
    ]

  sh_cmd(*([CXX] +
           %w[-fPIC] +
           qt6_test_cxxflags +
           [
             "tests/test-qt6-callback-delivery.cpp",
             "modules/bridges/qt6/im-cim-qt.cpp"
           ] +
           BUILD[:qt6_libs] +
           embedded_archive_link_flags +
           CIM_LINK_LIBS +
           CLAIR_LINK_CLOSURE +
           clair_rpath_flags +
           [
             "-lpthread",
             lib_rpath_flag,
             "-o",
             TEST_QT6_CALLBACK_EXE
           ]))
end

file TEST_NESTED_HOST_EXE => [
  NESTED_INNER_PLUGIN,
  *NESTED_OUTER_MODULES.keys,
  :nested_shared_cim,
  "tests/test-nested-host.c"
] do
  unless NESTED_HOST_TEST_ENABLED
    abort "nested host tests require static ELF embedding"
  end

  ensure_native_task("nested host test executable build")
  mkdir_p BIN_DIR

  sh_cmd(*([CC] +
           TEST_CFLAGS +
           ["tests/test-nested-host.c"] +
           DYNAMIC_LOADER_LIBS +
           [
             "-lpthread",
             "-o",
             TEST_NESTED_HOST_EXE
           ]))
end

test_executable_targets = [
  CIM_ADA_TEST_EXE,
  TEST_CIM_EXE,
  TEST_LIBHANGUL_EXE
]
test_executable_targets << TEST_NESTED_HOST_EXE if NESTED_HOST_TEST_ENABLED
test_executable_targets << TEST_GTK_PREEDIT_EXE if BUILD[:gtk3_enabled]
test_executable_targets << TEST_GTK_CANDIDATE_EXE if BUILD[:gtk3_enabled]
test_executable_targets << TEST_GTK_CALLBACK_EXE if BUILD[:gtk3_enabled]
test_executable_targets << TEST_QT6_CALLBACK_EXE if BUILD[:qt6_enabled]

desc "Build test executables"
task :tests => test_executable_targets

desc "Run the consolidated test suite"
task :test do
  ensure_native_task("test")
  Rake::Task[:build].invoke

  test_env = {
    "CIM_TEST_PLUGIN_DIR" => File.expand_path(LIB_DIR),
    "CIM_TEST_CIM_EXECUTABLE" => File.expand_path(TEST_CIM_EXE),
    "CIM_TEST_LIBHANGUL_EXECUTABLE" =>
      File.expand_path(TEST_LIBHANGUL_EXE)
  }

  if NESTED_HOST_TEST_ENABLED
    test_env["CIM_TEST_NESTED_HOST_EXECUTABLE"] =
      File.expand_path(TEST_NESTED_HOST_EXE)
    test_env["CIM_TEST_SHARED_CIM"] =
      File.expand_path(NESTED_SHARED_CIM)
  end

  if BUILD[:gtk3_enabled]
    test_env["CIM_TEST_GTK_PREEDIT_EXECUTABLE"] =
      File.expand_path(TEST_GTK_PREEDIT_EXE)
    test_env["CIM_TEST_GTK_CANDIDATE_EXECUTABLE"] =
      File.expand_path(TEST_GTK_CANDIDATE_EXE)
    test_env["CIM_TEST_GTK_CALLBACK_EXECUTABLE"] =
      File.expand_path(TEST_GTK_CALLBACK_EXE)
  end

  if BUILD[:qt6_enabled]
    test_env["CIM_TEST_QT6_CALLBACK_EXECUTABLE"] =
      File.expand_path(TEST_QT6_CALLBACK_EXE)
  end

  if LIB_TYPE == "relocatable" || NESTED_HOST_TEST_ENABLED
    library_path_name =
      case TARGET_OS
      when "darwin"
        "DYLD_LIBRARY_PATH"
      when "windows"
        "PATH"
      else
        "LD_LIBRARY_PATH"
      end

    library_paths = [
      File.expand_path(LIB_DIR),
      File.expand_path(CLAIR_LIB_DIR),
      ENV[library_path_name]
    ].compact.reject(&:empty?)
    test_env[library_path_name] = library_paths.join(File::PATH_SEPARATOR)
  end

  sh test_env, "./#{CIM_ADA_TEST_EXE}"
end

desc "Aggregates specified directories or files into a single output text file."
task :plat do
  targets = ARGV.drop(1)
  abort "Usage: rake plat <target1> [target2 ...]" if targets.empty?

  stamp = Time.now.strftime('%Y-%m-%d-%H-%M-%S')
  output = "#{stamp}.txt"

  files = targets.flat_map do |t|
    if File.file?(t)
      t
    elsif File.directory?(t)
      Dir.glob("#{t}/**/*").select { |f| File.file?(f) }
    else
      puts "Warning: Target not found or invalid - #{t}"
      []
    end
  end.uniq

  abort "No files found in #{targets.join(', ')}." if files.empty?

  File.open(output, "w") do |f|
    files.each do |path|
      f.puts "=" * 80
      f.puts "File: #{path}"
      f.puts "=" * 80
      begin
        f.puts File.read(path)
      rescue => e
        f.puts "-- I/O Error: #{e.message} --"
      end
      f.puts "\n\n"
    end
  end
  puts "Aggregation complete: #{output}"
  exit 0
end

desc "Clean build artifacts"
task :clean do
  begin
    gprclean_project("cim.gpr")
  rescue RuntimeError => e
    puts "Warning: gprclean error: #{e.message}"
  end

  rm_rf [OBJ_PROFILE_DIR, LIB_DIR, BIN_DIR]
end

desc "Rebuild everything"
task :rebuild => [:clean, :build]

desc "Remove all build artifacts"
task :clobber do
  rm_rf BUILD_ROOT
  rm_rf %w[obj lib bin]
end

desc "Install Cim"
task :install => [GTK3_MODULE,
                  QT6_MODULE] do
  if BUILD[:gtk3_enabled]
    mkdir_p File.dirname(gtk3_module_path)

    sh_cmd("install",
           "-m",
           "755",
           GTK3_MODULE,
           gtk3_module_path)
  end

  if BUILD[:qt6_enabled]
    mkdir_p File.dirname(qt6_module_path)

    sh_cmd("install",
           "-m",
           "755",
           QT6_MODULE,
           qt6_module_path)
  end
end

desc "Uninstall Cim"
task :uninstall do
  if BUILD[:gtk3_enabled]
    sh_cmd("rm",
           "-f",
           gtk3_module_path)
  end

  if BUILD[:qt6_enabled]
    sh_cmd("rm",
           "-f",
           qt6_module_path)
  end
end

desc "Update GTK3 input method module cache"
task :"gtk3-update-cache" do
  unless BUILD[:gtk3_enabled]
    abort "GTK3 cache cannot be updated: gtk-query-immodules-3.0 not found"
  end

  sh_cmd BUILD[:gtk_query_immodules3], "--update-cache"
end

desc "Default task"
task :default => :build
