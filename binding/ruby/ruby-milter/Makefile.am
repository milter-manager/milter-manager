SUBDIRS = 					\
	milter

EXTRA_DIST =					\
	milter.rb.in

if WITH_RUBY_MILTER
milter.rb: Makefile $(srcdir)/milter.rb.in
	sed							\
	  -e "s,\$$(binding_extdir),$(binding_extdir),g"	\
	  -e "s,\$$(binding_libdir),$(binding_libdir),g"	\
	  $(srcdir)/milter.rb.in > milter.rb

ruby_milter_DATA =	\
	milter.rb

BUILT_SOURCES = $(ruby_milter_DATA)
CLEANFILES = $(ruby_milter_DATA)
endif
