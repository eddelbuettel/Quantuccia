/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2006 StatPro Italia srl

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*! \file position.hpp
    \brief Short or long position
*/

#ifndef quantlib_position_hpp
#define quantlib_position_hpp

#include <ql/qldefines.hpp>
#include <iosfwd>

namespace QuantLib {

    struct Position {
        enum Type { Long, Short };
    };

    /*! \relates Position */
    std::ostream& operator<<(std::ostream&,
                             Position::Type);

}


/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2007 Cristina Duminuco

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

#include <ql/types.hpp>
#include <ql/errors.hpp>

namespace QuantLib {

    inline std::ostream& operator<<(std::ostream& out, Position::Type p) {
        switch (p) {
          case Position::Long :
            return out << "Long";
          case Position::Short :
            return out << "Short";
          default:
            QL_FAIL("unknown Position Type (" << Integer(p) << ")");
        }
    }

}


#endif

