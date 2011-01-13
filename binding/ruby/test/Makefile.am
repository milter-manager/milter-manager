SUBDIRS =	\
	toolkit	\
	manager

TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes

utils_files =					\
	milter-test-utils.rb			\
	milter-encoder-test-utils.rb		\
	milter-manager-encoder-test-utils.rb	\
	milter-detector-test-utils.rb		\
	milter-parse-test-utils.rb		\
	milter-event-loop-test-utils.rb		\
	milter-multiprocess-test-utils.rb

EXTRA_DIST =		\
	$(TESTS)	\
	run-test.rb	\
	$(utils_files)

echo-abs-top-builddir:
	@echo $(abs_top_builddir)
