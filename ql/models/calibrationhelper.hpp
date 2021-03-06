/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2001, 2002, 2003 Sadruddin Rejeb
 Copyright (C) 2015 Peter Caspers

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

/*! \file calibrationhelper.hpp
    \brief Calibration helper class
*/

#ifndef quantlib_interest_rate_modelling_calibration_helper_h
#define quantlib_interest_rate_modelling_calibration_helper_h

#include <ql/quote.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>
#include <ql/termstructures/volatility/volatilitytype.hpp>
#include <ql/patterns/lazyobject.hpp>
#include <list>

namespace QuantLib {

    class PricingEngine;

    //! liquid market instrument used during calibration
    class CalibrationHelper : public LazyObject {
      public:
        enum CalibrationErrorType {
                            RelativePriceError, PriceError, ImpliedVolError};
        CalibrationHelper(const Handle<Quote>& volatility,
                          const Handle<YieldTermStructure>& termStructure,
                          CalibrationErrorType calibrationErrorType
                                                         = RelativePriceError,
                          const VolatilityType type = ShiftedLognormal,
                          const Real shift = 0.0)
        : volatility_(volatility), termStructure_(termStructure),
          volatilityType_(type), shift_(shift),
          calibrationErrorType_(calibrationErrorType) {
            registerWith(volatility_);
            registerWith(termStructure_);
        }

        void performCalculations() const {
            marketValue_ = blackPrice(volatility_->value());
        }

        //! returns the volatility Handle
        Handle<Quote> volatility() const { return volatility_; }

        //! returns the volatility type
        VolatilityType volatilityType() const { return volatilityType_; }

        //! returns the actual price of the instrument (from volatility)
        Real marketValue() const { calculate(); return marketValue_; }

        //! returns the price of the instrument according to the model
        virtual Real modelValue() const = 0;

        //! returns the error resulting from the model valuation
        virtual Real calibrationError();

        virtual void addTimesTo(std::list<Time>& times) const = 0;

        //! Black volatility implied by the model
        Volatility impliedVolatility(Real targetValue,
                                     Real accuracy,
                                     Size maxEvaluations,
                                     Volatility minVol,
                                     Volatility maxVol) const;

        //! Black or Bachelier price given a volatility
        virtual Real blackPrice(Volatility volatility) const = 0;

        void setPricingEngine(const boost::shared_ptr<PricingEngine>& engine) {
            engine_ = engine;
        }

      protected:
        mutable Real marketValue_;
        Handle<Quote> volatility_;
        Handle<YieldTermStructure> termStructure_;
        boost::shared_ptr<PricingEngine> engine_;
        const VolatilityType volatilityType_;
        const Real shift_;

      private:
        class ImpliedVolatilityHelper;
        const CalibrationErrorType calibrationErrorType_;
    };

}


/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2001, 2002, 2003 Sadruddin Rejeb
 Copyright (C) 2015 Peter Caspers

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

#include <ql/math/solvers1d/brent.hpp>

namespace QuantLib {

    class CalibrationHelper::ImpliedVolatilityHelper {
      public:
        ImpliedVolatilityHelper(const CalibrationHelper& helper,
                                Real value)
        : helper_(helper), value_(value) {}

        Real operator()(Volatility x) const {
            return value_ - helper_.blackPrice(x);
        }
      private:
        const CalibrationHelper& helper_;
        Real value_;
    };

    inline Volatility CalibrationHelper::impliedVolatility(Real targetValue,
                                                    Real accuracy,
                                                    Size maxEvaluations,
                                                    Volatility minVol,
                                                    Volatility maxVol) const {

        ImpliedVolatilityHelper f(*this,targetValue);
        Brent solver;
        solver.setMaxEvaluations(maxEvaluations);
        return solver.solve(f,accuracy,volatility_->value(),minVol,maxVol);
    }

    inline Real CalibrationHelper::calibrationError() {
        Real error;
        
        switch (calibrationErrorType_) {
          case RelativePriceError:
            error = std::fabs(marketValue() - modelValue())/marketValue();
            break;
          case PriceError:
            error = marketValue() - modelValue();
            break;
          case ImpliedVolError: 
            {
              Real minVol = volatilityType_ == ShiftedLognormal ? 0.0010 : 0.00005;
              Real maxVol = volatilityType_ == ShiftedLognormal ? 10.0 : 0.50;
              const Real lowerPrice = blackPrice(minVol);
              const Real upperPrice = blackPrice(maxVol);
              const Real modelPrice = modelValue();

              Volatility implied;
              if (modelPrice <= lowerPrice)
                  implied = minVol;
              else if (modelPrice >= upperPrice)
                  implied = maxVol;
              else
                  implied = this->impliedVolatility(
                                          modelPrice, 1e-12, 5000, minVol, maxVol);
              error = implied - volatility_->value();
            }
            break;
          default:
            QL_FAIL("unknown Calibration Error Type");
        }
        
        return error;
    }
}


#endif
