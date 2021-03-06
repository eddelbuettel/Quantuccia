/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2012 Ralph Schreyer
 Copyright (C) 2012 Mateusz Kapturski

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

/*! \file differentialevolution.hpp
    \brief Differential Evolution optimization method
*/

#ifndef quantlib_optimization_differential_evolution_hpp
#define quantlib_optimization_differential_evolution_hpp

#include <ql/math/optimization/constraint.hpp>
#include <ql/math/optimization/problem.hpp>
#include <ql/math/randomnumbers/mt19937uniformrng.hpp>

namespace QuantLib {

    //! Differential Evolution configuration object
    /*! The algorithm and strategy names are taken from here:

        Price, K., Storn, R., 1997. Differential Evolution -
        A Simple and Efficient Heuristic for Global Optimization
        over Continuous Spaces.
        Journal of Global Optimization, Kluwer Academic Publishers,
        1997, Vol. 11, pp. 341 - 359.

        There are seven basic strategies for creating mutant population
        currently implemented. Three basic crossover types are also
        available.

        Future development:
        1) base element type to be extracted
        2) L differences to be used instead of fixed number
        3) various weights distributions for the differences (dither etc.)
        4) printFullInfo parameter usage to track the algorithm

        \warning This was reported to fail tests on Mac OS X 10.8.4.
    */


    //! %OptimizationMethod using Differential Evolution algorithm
    /*! \ingroup optimizers */
    class DifferentialEvolution: public OptimizationMethod {
      public:
        enum Strategy {
            Rand1Standard,
            BestMemberWithJitter,
            CurrentToBest2Diffs,
            Rand1DiffWithPerVectorDither,
            Rand1DiffWithDither,
            EitherOrWithOptimalRecombination,
            Rand1SelfadaptiveWithRotation
        };
        enum CrossoverType {
            Normal,
            Binomial,
            Exponential
        };

        struct Candidate {
            Array values;
            Real cost;
            Candidate(Size size = 0) : values(size, 0.0), cost(0.0) {}
        };

        class Configuration {
          public:
            Strategy strategy;
            CrossoverType crossoverType;
            Size populationMembers;
            Real stepsizeWeight, crossoverProbability;
            unsigned long seed;
            bool applyBounds, crossoverIsAdaptive;

            Configuration()
            : strategy(BestMemberWithJitter),
              crossoverType(Normal),
              populationMembers(100),
              stepsizeWeight(0.2),
              crossoverProbability(0.9),
              seed(0),
              applyBounds(true),
              crossoverIsAdaptive(false) {}

            Configuration& withBounds(bool b = true) {
                applyBounds = b;
                return *this;
            }

            Configuration& withCrossoverProbability(Real p) {
                QL_REQUIRE(p>=0.0 && p<=1.0,
                          "Crossover probability (" << p
                           << ") must be in [0,1] range");
                crossoverProbability = p;
                return *this;
            }

            Configuration& withPopulationMembers(Size n) {
                QL_REQUIRE(n>0, "Positive number of population members required");
                populationMembers = n;
                return *this;
            }

            Configuration& withSeed(unsigned long s) {
                seed = s;
                return *this;
            }

            Configuration& withAdaptiveCrossover(bool b = true) {
                crossoverIsAdaptive = b;
                return *this;
            }

            Configuration& withStepsizeWeight(Real w) {
                QL_ENSURE(w>=0 && w<=2.0,
                          "Step size weight ("<< w
                          << ") must be in [0,2] range");
                stepsizeWeight = w;
                return *this;
            }

            Configuration& withCrossoverType(CrossoverType t) {
                crossoverType = t;
                return *this;
            }

            Configuration& withStrategy(Strategy s) {
                strategy = s;
                return *this;
            }
        };


        DifferentialEvolution(const Configuration& configuration = Configuration())
        : configuration_(configuration), rng_(configuration.seed) {}

        virtual EndCriteria::Type minimize(Problem& p,
                                           const EndCriteria& endCriteria);

        const Configuration& configuration() const {
            return configuration_;
        }

      private:
        Configuration configuration_;
        Array upperBound_, lowerBound_;
        mutable Array currGenSizeWeights_, currGenCrossover_;
        Candidate bestMemberEver_;
        MersenneTwisterUniformRng rng_;

        void fillInitialPopulation(std::vector<Candidate>& population,
                                   const Problem& p) const;

        void getCrossoverMask(std::vector<Array>& crossoverMask,
                              std::vector<Array>& invCrossoverMask,
                              const Array& mutationProbabilities) const;

        Array getMutationProbabilities(
                              const std::vector<Candidate>& population) const;

        void adaptSizeWeights() const;

        void adaptCrossover() const;

        void calculateNextGeneration(std::vector<Candidate>& population,
                                     const CostFunction& costFunction) const;

        Array rotateArray(Array inputArray) const;

        void crossover(const std::vector<Candidate>& oldPopulation,
                       std::vector<Candidate> & population,
                       const std::vector<Candidate>& mutantPopulation,
                       const std::vector<Candidate>& mirrorPopulation,
                       const CostFunction& costFunction) const;
    };

}


/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2012 Ralph Schreyer
 Copyright (C) 2012 Mateusz Kapturski

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

namespace QuantLib {

    namespace {

        struct sort_by_cost {
            bool operator()(const DifferentialEvolution::Candidate& left,
                            const DifferentialEvolution::Candidate& right) {
                return left.cost < right.cost;
            }
        };

    }

    inline EndCriteria::Type DifferentialEvolution::minimize(Problem& p, const EndCriteria& endCriteria) {
        EndCriteria::Type ecType;

        upperBound_ = p.constraint().upperBound(p.currentValue());
        lowerBound_ = p.constraint().lowerBound(p.currentValue());
        currGenSizeWeights_ = Array(configuration().populationMembers,
                                    configuration().stepsizeWeight);
        currGenCrossover_ = Array(configuration().populationMembers,
                                  configuration().crossoverProbability);

        std::vector<Candidate> population(configuration().populationMembers,
                                          Candidate(p.currentValue().size()));
        fillInitialPopulation(population, p);

        std::partial_sort(population.begin(), population.begin() + 1, population.end(),
                          sort_by_cost());
        bestMemberEver_ = population.front();
        Real fxOld = population.front().cost;
        Size iteration = 0, stationaryPointIteration = 0;

        // main loop - calculate consecutive emerging populations
        while (!endCriteria.checkMaxIterations(iteration++, ecType)) {
            calculateNextGeneration(population, p.costFunction());
            std::partial_sort(population.begin(), population.begin() + 1, population.end(),
                              sort_by_cost());
            if (population.front().cost < bestMemberEver_.cost)
                bestMemberEver_ = population.front();
            Real fxNew = population.front().cost;
            if (endCriteria.checkStationaryFunctionValue(fxOld, fxNew, stationaryPointIteration,
                                                         ecType))
                break;
            fxOld = fxNew;
        };
        p.setCurrentValue(bestMemberEver_.values);
        p.setFunctionValue(bestMemberEver_.cost);
        return ecType;
    }

    inline void DifferentialEvolution::calculateNextGeneration(
                                     std::vector<Candidate>& population,
                                     const CostFunction& costFunction) const {

        std::vector<Candidate> mirrorPopulation;
        std::vector<Candidate> oldPopulation = population;

        switch (configuration().strategy) {

          case Rand1Standard: {
              std::random_shuffle(population.begin(), population.end());
              std::vector<Candidate> shuffledPop1 = population;
              std::random_shuffle(population.begin(), population.end());
              std::vector<Candidate> shuffledPop2 = population;
              std::random_shuffle(population.begin(), population.end());
              mirrorPopulation = shuffledPop1;

              for (Size popIter = 0; popIter < population.size(); popIter++) {
                  population[popIter].values = population[popIter].values
                      + configuration().stepsizeWeight
                      * (shuffledPop1[popIter].values - shuffledPop2[popIter].values);
              }
          }
            break;

          case BestMemberWithJitter: {
              std::random_shuffle(population.begin(), population.end());
              std::vector<Candidate> shuffledPop1 = population;
              std::random_shuffle(population.begin(), population.end());
              Array jitter(population[0].values.size(), 0.0);

              for (Size popIter = 0; popIter < population.size(); popIter++) {
                  for (Size jitterIter = 0; jitterIter < jitter.size(); jitterIter++) {
                      jitter[jitterIter] = rng_.nextReal();
                  }
                  population[popIter].values = bestMemberEver_.values
                      + (shuffledPop1[popIter].values - population[popIter].values)
                      * (0.0001 * jitter + configuration().stepsizeWeight);
              }
              mirrorPopulation = std::vector<Candidate>(population.size(),
                                                        bestMemberEver_);
          }
            break;

          case CurrentToBest2Diffs: {
              std::random_shuffle(population.begin(), population.end());
              std::vector<Candidate> shuffledPop1 = population;
              std::random_shuffle(population.begin(), population.end());

              for (Size popIter = 0; popIter < population.size(); popIter++) {
                  population[popIter].values = oldPopulation[popIter].values
                      + configuration().stepsizeWeight
                      * (bestMemberEver_.values - oldPopulation[popIter].values)
                      + configuration().stepsizeWeight
                      * (population[popIter].values - shuffledPop1[popIter].values);
              }
              mirrorPopulation = shuffledPop1;
          }
            break;

          case Rand1DiffWithPerVectorDither: {
              std::random_shuffle(population.begin(), population.end());
              std::vector<Candidate> shuffledPop1 = population;
              std::random_shuffle(population.begin(), population.end());
              std::vector<Candidate> shuffledPop2 = population;
              std::random_shuffle(population.begin(), population.end());
              mirrorPopulation = shuffledPop1;
              Array FWeight = Array(population.front().values.size(), 0.0);
              for (Size fwIter = 0; fwIter < FWeight.size(); fwIter++)
                  FWeight[fwIter] = (1.0 - configuration().stepsizeWeight)
                      * rng_.nextReal() + configuration().stepsizeWeight;
              for (Size popIter = 0; popIter < population.size(); popIter++) {
                  population[popIter].values = population[popIter].values
                      + FWeight * (shuffledPop1[popIter].values - shuffledPop2[popIter].values);
              }
          }
            break;

          case Rand1DiffWithDither: {
              std::random_shuffle(population.begin(), population.end());
              std::vector<Candidate> shuffledPop1 = population;
              std::random_shuffle(population.begin(), population.end());
              std::vector<Candidate> shuffledPop2 = population;
              std::random_shuffle(population.begin(), population.end());
              mirrorPopulation = shuffledPop1;
              Real FWeight = (1.0 - configuration().stepsizeWeight) * rng_.nextReal()
                  + configuration().stepsizeWeight;
              for (Size popIter = 0; popIter < population.size(); popIter++) {
                  population[popIter].values = population[popIter].values
                      + FWeight * (shuffledPop1[popIter].values - shuffledPop2[popIter].values);
              }
          }
            break;

          case EitherOrWithOptimalRecombination: {
              std::random_shuffle(population.begin(), population.end());
              std::vector<Candidate> shuffledPop1 = population;
              std::random_shuffle(population.begin(), population.end());
              std::vector<Candidate> shuffledPop2 = population;
              std::random_shuffle(population.begin(), population.end());
              mirrorPopulation = shuffledPop1;
              Real probFWeight = 0.5;
              if (rng_.nextReal() < probFWeight) {
                  for (Size popIter = 0; popIter < population.size(); popIter++) {
                      population[popIter].values = oldPopulation[popIter].values
                          + configuration().stepsizeWeight
                          * (shuffledPop1[popIter].values - shuffledPop2[popIter].values);
                  }
              } else {
                  Real K = 0.5 * (configuration().stepsizeWeight + 1); // invariant with respect to probFWeight used
                  for (Size popIter = 0; popIter < population.size(); popIter++) {
                      population[popIter].values = oldPopulation[popIter].values
                          + K
                          * (shuffledPop1[popIter].values - shuffledPop2[popIter].values
                             - 2.0 * population[popIter].values);
                  }
              }
          }
            break;

          case Rand1SelfadaptiveWithRotation: {
              std::random_shuffle(population.begin(), population.end());
              std::vector<Candidate> shuffledPop1 = population;
              std::random_shuffle(population.begin(), population.end());
              std::vector<Candidate> shuffledPop2 = population;
              std::random_shuffle(population.begin(), population.end());
              mirrorPopulation = shuffledPop1;

              adaptSizeWeights();

              for (Size popIter = 0; popIter < population.size(); popIter++) {
                  if (rng_.nextReal() < 0.1){
                      population[popIter].values = rotateArray(bestMemberEver_.values);
                  }else {
                      population[popIter].values = bestMemberEver_.values
                          + currGenSizeWeights_[popIter]
                          * (shuffledPop1[popIter].values - shuffledPop2[popIter].values);
                  }
              }
          }
            break;

          default:
            QL_FAIL("Unknown strategy ("
                    << Integer(configuration().strategy) << ")");
        }
        // in order to avoid unnecessary copying we use the same population object for mutants
        crossover(oldPopulation, population, population, mirrorPopulation,
                  costFunction);
    }

    inline void DifferentialEvolution::crossover(
                               const std::vector<Candidate>& oldPopulation,
                               std::vector<Candidate>& population,
                               const std::vector<Candidate>& mutantPopulation,
                               const std::vector<Candidate>& mirrorPopulation,
                               const CostFunction& costFunction) const {

        if (configuration().crossoverIsAdaptive) {
            adaptCrossover();
        }

        Array mutationProbabilities = getMutationProbabilities(population);

        std::vector<Array> crossoverMask(population.size(),
                                         Array(population.front().values.size(), 1.0));
        std::vector<Array> invCrossoverMask = crossoverMask;
        getCrossoverMask(crossoverMask, invCrossoverMask, mutationProbabilities);

        // crossover of the old and mutant population
        for (Size popIter = 0; popIter < population.size(); popIter++) {
            population[popIter].values = oldPopulation[popIter].values * invCrossoverMask[popIter]
                + mutantPopulation[popIter].values * crossoverMask[popIter];
            // immediately apply bounds if specified
            if (configuration().applyBounds) {
                for (Size memIter = 0; memIter < population[popIter].values.size(); memIter++) {
                    if (population[popIter].values[memIter] > upperBound_[memIter])
                        population[popIter].values[memIter] = upperBound_[memIter]
                            + rng_.nextReal()
                            * (mirrorPopulation[popIter].values[memIter]
                               - upperBound_[memIter]);
                    if (population[popIter].values[memIter] < lowerBound_[memIter])
                        population[popIter].values[memIter] = lowerBound_[memIter]
                            + rng_.nextReal()
                            * (mirrorPopulation[popIter].values[memIter]
                               - lowerBound_[memIter]);
                }
            }
            // evaluate objective function as soon as possible to avoid unnecessary loops
            try {
                population[popIter].cost = costFunction.value(population[popIter].values);
            } catch (Error&) {
                population[popIter].cost = QL_MAX_REAL;
            }
        }
    }

    inline void DifferentialEvolution::getCrossoverMask(
                                  std::vector<Array> & crossoverMask,
                                  std::vector<Array> & invCrossoverMask,
                                  const Array & mutationProbabilities) const {
        for (Size cmIter = 0; cmIter < crossoverMask.size(); cmIter++) {
            for (Size memIter = 0; memIter < crossoverMask[cmIter].size(); memIter++) {
                if (rng_.nextReal() < mutationProbabilities[cmIter]) {
                    invCrossoverMask[cmIter][memIter] = 0.0;
                } else {
                    crossoverMask[cmIter][memIter] = 0.0;
                }
            }
        }
    }

    inline Array DifferentialEvolution::getMutationProbabilities(
                            const std::vector<Candidate> & population) const {
        Array mutationProbabilities = currGenCrossover_;
        switch (configuration().crossoverType) {
          case Normal:
            break;
          case Binomial:
            mutationProbabilities = currGenCrossover_
                * (1.0 - 1.0 / population.front().values.size())
                + 1.0 / population.front().values.size();
            break;
          case Exponential:
            for (Size coIter = 0;coIter< currGenCrossover_.size(); coIter++){
                mutationProbabilities[coIter] =
                    (1.0 - std::pow(currGenCrossover_[coIter],
                                    (int) population.front().values.size()))
                    / (population.front().values.size()
                       * (1.0 - currGenCrossover_[coIter]));
            }
            break;
          default:
            QL_FAIL("Unknown crossover type ("
                    << Integer(configuration().crossoverType) << ")");
            break;
        }
        return mutationProbabilities;
    }

    inline Array DifferentialEvolution::rotateArray(Array a) const {
        std::random_shuffle(a.begin(), a.end());
        return a;
    }

    inline void DifferentialEvolution::adaptSizeWeights() const {
        // [=Fl & =Fu] respectively see Brest, J. et al., 2006,
        // "Self-Adapting Control Parameters in Differential
        // Evolution"
        Real sizeWeightLowerBound = 0.1, sizeWeightUpperBound = 0.9;
         // [=tau1] A Comparative Study on Numerical Benchmark
         // Problems." page 649 for reference
        Real sizeWeightChangeProb = 0.1;
        for (Size coIter = 0;coIter < currGenSizeWeights_.size(); coIter++){
            if (rng_.nextReal() < sizeWeightChangeProb)
                currGenSizeWeights_[coIter] = sizeWeightLowerBound + rng_.nextReal() * sizeWeightUpperBound;
        }
    }

    inline void DifferentialEvolution::adaptCrossover() const {
        Real crossoverChangeProb = 0.1; // [=tau2]
        for (Size coIter = 0;coIter < currGenCrossover_.size(); coIter++){
            if (rng_.nextReal() < crossoverChangeProb)
                currGenCrossover_[coIter] = rng_.nextReal();
        }
    }

    inline void DifferentialEvolution::fillInitialPopulation(
                                          std::vector<Candidate> & population,
                                          const Problem& p) const {

        // use initial values provided by the user
        population.front().values = p.currentValue();
        population.front().cost = p.costFunction().value(population.front().values);
        // rest of the initial population is random
        for (Size j = 1; j < population.size(); ++j) {
            for (Size i = 0; i < p.currentValue().size(); ++i) {
                Real l = lowerBound_[i], u = upperBound_[i];
                population[j].values[i] = l + (u-l)*rng_.nextReal();
            }
            population[j].cost = p.costFunction().value(population[j].values);
        }
    }

}


#endif