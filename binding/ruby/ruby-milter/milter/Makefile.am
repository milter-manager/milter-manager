EXTRA_DIST =					\
	core.rb.in				\
	client.rb.in				\
	server.rb.in

if WITH_RUBY_MILTER
core.rb: Makefile $(srcdir)/core.rb.in
	sed							\
	  -e "s,\$$(binding_extdir),$(binding_extdir),g"	\
	  -e "s,\$$(binding_libdir),$(binding_libdir),g"	\
	  $(srcdir)/core.rb.in > core.rb

client.rb: Makefile $(srcdir)/client.rb.in
	sed							\
	  -e "s,\$$(binding_extdir),$(binding_extdir),g"	\
	  -e "s,\$$(binding_libdir),$(binding_libdir),g"	\
	  $(srcdir)/client.rb.in > client.rb

server.rb: Makefile $(srcdir)/server.rb.in
	sed							\
	  -e "s,\$$(binding_extdir),$(binding_extdir),g"	\
	  -e "s,\$$(binding_libdir),$(binding_libdir),g"	\
	  $(srcdir)/server.rb.in > server.rb

ruby_milter_milterdir = $(ruby_milterdir)/milter
ruby_milter_milter_DATA =			\
	core.rb					\
	client.rb				\
	server.rb

BUILT_SOURCES = $(ruby_milter_milter_DATA)
CLEANFILES = $(ruby_milter_milter_DATA)
endif
