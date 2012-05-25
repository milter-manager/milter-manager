SUBDIRS =	\
	core	\
	client	\
	server	\
	manager

TESTS = run-test.sh
TESTS_ENVIRONMENT =				\
	NO_MAKE=yes				\
	RUBY="$(RUBY)"				\
	abs_top_builddir="$(abs_top_builddir)"	\
	abs_top_srcdir="$(abs_top_srcdir)"

utils_files =					\
	milter-test-utils.rb			\
	milter-encoder-test-utils.rb		\
	milter-manager-encoder-test-utils.rb	\
	milter-detector-test-utils.rb		\
	milter-parse-test-utils.rb		\
	milter-event-loop-test-utils.rb		\
	milter-multi-process-test-utils.rb

fixture_files =					\
	fixtures/raw-shift_jis-subject.eml	\
	fixtures/shift_jis-8bit.eml

EXTRA_DIST =					\
	$(TESTS)				\
	run-test.rb				\
	$(utils_files)				\
	$(fixture_files)

echo-abs-top-builddir:
	@echo $(abs_top_builddir)
