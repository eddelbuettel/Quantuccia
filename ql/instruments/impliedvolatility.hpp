/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2007, 2009 StatPro Italia srl

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

/*! \file impliedvolatility.hpp
    \brief Utilities for implied-volatility calculation
*/

#ifndef quantlib_implied_volatility_hpp
#define quantlib_implied_volatility_hpp

#include <ql/instrument.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/processes/blackscholesprocess.hpp>

namespace QuantLib {

    namespace detail {

        //! helper class for one-asset implied-volatility calculation
        /*! The passed engine must be linked to the passed quote (see,
             e.g., VanillaOption to see how this can be achieved.)

             \note this function is meant for developers of option
                   classes so that they can implement an
                   impliedVolatility() method.
        */
        class ImpliedVolatilityHelper {
          public:
            static Volatility calculate(const Instrument& instrument,
                                        const PricingEngine& engine,
                                        SimpleQuote& volQuote,
                                        Real targetValue,
                                        Real accuracy,
                                        Natural maxEvaluations,
                                        Volatility minVol,
                                        Volatility maxVol);
            // utilities

            /*! The returned process is equal to the passed one, except
                for the volatility which is flat and whose value is driven
                by the passed quote.
            */
            static boost::shared_ptr<GeneralizedBlackScholesProcess> clone(
                     const boost::shared_ptr<GeneralizedBlackScholesProcess>&,
                     const boost::shared_ptr<SimpleQuote>&);
        };

    }

}


/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2007, 2009 StatPro Italia srl

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

#include <ql/termstructures/volatility/equityfx/blackconstantvol.hpp>
#include <ql/math/solvers1d/brent.hpp>

namespace QuantLib {

    namespace {

        class PriceError {
          public:
            PriceError(const PricingEngine& engine,
                       SimpleQuote& vol,
                       Real targetValue);
            Real operator()(Volatility x) const;
          private:
            const PricingEngine& engine_;
            SimpleQuote& vol_;
            Real targetValue_;
            const Instrument::results* results_;
        };

        inline PriceError::PriceError(const PricingEngine& engine,
                               SimpleQuote& vol,
                               Real targetValue)
        : engine_(engine), vol_(vol), targetValue_(targetValue) {
            results_ =
                dynamic_cast<const Instrument::results*>(engine_.getResults());
            QL_REQUIRE(results_ != 0,
                       "pricing engine does not supply needed results");
        }

        inline Real PriceError::operator()(Volatility x) const {
            vol_.setValue(x);
            engine_.calculate();
            return results_->value-targetValue_;
        }

    }


    namespace detail {

        inline Volatility ImpliedVolatilityHelper::calculate(
                                                 const Instrument& instrument,
                                                 const PricingEngine& engine,
                                                 SimpleQuote& volQuote,
                                                 Real targetValue,
                                                 Real accuracy,
                                                 Natural maxEvaluations,
                                                 Volatility minVol,
                                                 Volatility maxVol) {

            instrument.setupArguments(engine.getArguments());
            engine.getArguments()->validate();

            PriceError f(engine, volQuote, targetValue);
            Brent solver;
            solver.setMaxEvaluations(maxEvaluations);
            Volatility guess = (minVol+maxVol)/2.0;
            Volatility result = solver.solve(f, accuracy, guess,
                                             minVol, maxVol);
            return result;
        }

        inline boost::shared_ptr<GeneralizedBlackScholesProcess>
        ImpliedVolatilityHelper::clone(
             const boost::shared_ptr<GeneralizedBlackScholesProcess>& process,
             const boost::shared_ptr<SimpleQuote>& volQuote) {

            Handle<Quote> stateVariable = process->stateVariable();
            Handle<YieldTermStructure> dividendYield = process->dividendYield();
            Handle<YieldTermStructure> riskFreeRate = process->riskFreeRate();

            Handle<BlackVolTermStructure> blackVol = process->blackVolatility();
            Handle<BlackVolTermStructure> volatility(
                boost::shared_ptr<BlackVolTermStructure>(
                               new BlackConstantVol(blackVol->referenceDate(),
                                                    blackVol->calendar(),
                                                    Handle<Quote>(volQuote),
                                                    blackVol->dayCounter())));

            return boost::shared_ptr<GeneralizedBlackScholesProcess>(
                new GeneralizedBlackScholesProcess(stateVariable, dividendYield,
                                                   riskFreeRate, volatility));
        }

    }

}

#endif