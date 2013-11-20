eixx_LTLIBRARIES = libeixx.la

eixx_hdrs1 =  alloc_pool.hpp \
              alloc_pool_st.hpp \
              alloc_std_debug.hpp \
              alloc_std.hpp \
		config.h \
              connect.hpp \
              eixx.hpp \
              eterm.hpp \
		eterm_exception.hpp
eixx_hdrs3 =  marshal/alloc_base.hpp \
              marshal/atom.hpp \
              marshal/binary.hpp \
              marshal/binary.ipp \
              marshal/defaults.hpp \
              marshal/endian.hpp \
              marshal/eterm_format.hpp \
              marshal/eterm_format.ipp \
              marshal/eterm.hpp \
              marshal/eterm.ipp \
              marshal/eterm_match.hpp \
              marshal/list.hpp \
              marshal/list.ipp \
              marshal/pid.hpp \
              marshal/pid.ipp \
              marshal/port.hpp \
              marshal/port.ipp \
              marshal/ref.hpp \
              marshal/ref.ipp \
              marshal/string.hpp \
              marshal/trace.hpp \
              marshal/tuple.hpp \
              marshal/tuple.ipp \
              marshal/varbind.hpp \
              marshal/var.hpp \
              marshal/visit_encoder.hpp \
              marshal/visit_encode_size.hpp \
              marshal/visit.hpp \
              marshal/visit_match.hpp \
              marshal/visit_subst.hpp \
              marshal/visit_to_string.hpp
eixx_hdrs4 = \
              util/common.hpp \
              util/hashtable.hpp \
              util/sync.hpp \
              util/string_util.hpp

eixx_headers= $(eixx_hdrs1) \
			  $(eixx_hdrs3) \
			  $(eixx_hdrs4)

libeixx_la_hdr1dir		 = $(includedir)/@PACKAGE@
libeixx_la_hdr1_HEADERS  = $(eixx_hdrs1:%=../include/@PACKAGE@/%)
libeixx_la_hdr3dir		 = $(includedir)/@PACKAGE@/marshal
libeixx_la_hdr3_HEADERS  = $(eixx_hdrs3:%=../include/@PACKAGE@/%)
libeixx_la_hdr4dir		 = $(includedir)/@PACKAGE@/util
libeixx_la_hdr4_HEADERS  = $(eixx_hdrs4:%=../include/@PACKAGE@/%)

libeixx_la_SOURCES  = atom.cpp defaults.cpp \
					  $(eixx_headers:%=../include/@PACKAGE@/%)
libeixx_la_LTADD    = -version-info 0:1 -shared
libeixx_la_LIBS     = $(BOOST_THREAD_LIB) $(CRYPTO_LIBS)
libeixx_la_CXXFLAGS = $(if $(tr1),-std=c++0x) -g -O$(if $(optimize),3 -DBOOST_DISABLE_ASSERTS,0) -I../include $(BOOST_CPPFLAGS) \
					  $(EI_CPPFLAGS)
