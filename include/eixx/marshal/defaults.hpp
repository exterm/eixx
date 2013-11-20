//----------------------------------------------------------------------------
/// \file  defaults.hpp
//----------------------------------------------------------------------------
/// \brief Definition of some globally used types.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the eixx (Erlang C++ Interface) Library.

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/
#ifndef _EIXX_DEFAULTS_HPP
#define _EIXX_DEFAULTS_HPP

#ifndef EIXX_NAMESPACE
#define EIXX_NAMESPACE eixx
#endif

// Branch prediction optimization (see http://lwn.net/Articles/255364/)
#ifndef NO_HINT_BRANCH_PREDICTION
#  define unlikely(expr) __builtin_expect(!!(expr), 0)
#  define likely(expr)   __builtin_expect(!!(expr), 1)
#else
#  define unlikely(expr) (expr)
#  define likely(expr)   (expr)
#endif

namespace EIXX_NAMESPACE {
    namespace marshal {

        // Forward declarations
        template <typename Alloc> class eterm;
        template <typename Alloc> class tuple;
        template <typename Alloc> class list;

        namespace marshal {
            template <typename Alloc> struct visit_eterm_stringify;
            template <typename Alloc> struct visit_eterm_encode_size_calc;
        }

        /// Maximum and default sizes
        enum {
              DEF_HEADER_SIZE   = 4
        };

        template <typename T, typename Alloc> T& get(eterm<Alloc>& t);
    } // namespace marshal

    // eterm types
    enum eterm_type {
          UNDEFINED         = 0
        , LONG              = 1
        , DOUBLE            = 2
        , BOOL              = 3
        // ATOM is the first compound item having a constructor
        , ATOM              = 4
        , STRING            = 5
        , BINARY            = 6
        , PID               = 7
        , PORT              = 8
        , REF               = 9
        , VAR               = 10
        , TUPLE             = 11
        , LIST              = 12
        , TRACE             = 13
        , MAX_ETERM_TYPE    = 13
    };

    /// Returns string representation of type \a a_type.
    const char* type_to_string(eterm_type a_type);

} // namespace EIXX_NAMESPACE

#endif // _EIXX_DEFAULTS_HPP

