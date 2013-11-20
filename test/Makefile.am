bin_PROGRAMS = test_eterm

AM_CXXFLAGS = -I. -I$(srcdir)/../include $(BOOST_CPPFLAGS) \
   			  -I$(ERLANG_LIB_DIR_erl_interface)/include \
   			  -I$(ERLANG_LIB_DIR_erl_interface)/src

test_eterm_SOURCES   = test_eterm.cpp test_eterm_encode.cpp \
                       test_eterm_format.cpp test_eterm_match.cpp \
                       test_eterm_pool.cpp test_eterm_refc.cpp
test_eterm_CPPFLAGS  = -DBOOST_TEST_DYN_LINK -g -O$(if $(optimize),3 -DBOOST_DISABLE_ASSERTS,0) \
					   $(if $(debug),-DEIXX_DEBUG)
test_eterm_LDFLAGS 	= $(BOOST_LDFLAGS) \
					  -L$(ERLANG_LIB_DIR_erl_interface)/lib
test_eterm_LDADD	= -L../src/.libs -leixx -lei \
					  $(BOOST_SYSTEM_LIB) $(BOOST_UNIT_TEST_FRAMEWORK_LIB) \
					  $(BOOST_THREAD_LIB)
