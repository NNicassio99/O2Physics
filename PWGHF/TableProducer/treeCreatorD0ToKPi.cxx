// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file treeCreatorD0ToKPi.cxx
/// \brief Writer of the 2 prong candidates in the form of flat tables to be stored in TTrees.
///        Intended for debug or for the local optimization of analysis on small samples.
///        In this file are defined and filled the output tables
///
/// \author Nicolo' Jacazio <nicolo.jacazio@cern.ch>, CERN
/// \author Andrea Tavira García <tavira-garcia@ijclab.in2p3.fr>, IJCLab

#include "Framework/AnalysisTask.h"
#include "Framework/runDataProcessing.h"

#include "PWGHF/DataModel/CandidateReconstructionTables.h"
#include "PWGHF/DataModel/CandidateSelectionTables.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::framework::expressions;
using namespace o2::aod::hf_cand_2prong;

namespace o2::aod
{
namespace full
{
DECLARE_SOA_INDEX_COLUMN(Collision, collision);
DECLARE_SOA_COLUMN(RSecondaryVertex, rSecondaryVertex, float);
DECLARE_SOA_COLUMN(PtProng0, ptProng0, float);
DECLARE_SOA_COLUMN(PProng0, pProng0, float);
DECLARE_SOA_COLUMN(ImpactParameterNormalised0, impactParameterNormalised0, float);
DECLARE_SOA_COLUMN(PtProng1, ptProng1, float);
DECLARE_SOA_COLUMN(PProng1, pProng1, float);
DECLARE_SOA_COLUMN(ImpactParameterNormalised1, impactParameterNormalised1, float);
DECLARE_SOA_COLUMN(CandidateSelFlag, candidateSelFlag, int8_t);
DECLARE_SOA_COLUMN(M, m, float);
DECLARE_SOA_COLUMN(Pt, pt, float);
DECLARE_SOA_COLUMN(P, p, float);
DECLARE_SOA_COLUMN(Eta, eta, float);
DECLARE_SOA_COLUMN(Phi, phi, float);
DECLARE_SOA_COLUMN(Y, y, float);
DECLARE_SOA_COLUMN(E, e, float);
DECLARE_SOA_COLUMN(NSigTpcPi0, nSigTpcPi0, float);
DECLARE_SOA_COLUMN(NSigTpcKa0, nSigTpcKa0, float);
DECLARE_SOA_COLUMN(NSigTofPi0, nSigTofPi0, float);
DECLARE_SOA_COLUMN(NSigTofKa0, nSigTofKa0, float);
DECLARE_SOA_COLUMN(NSigTpcPi1, nSigTpcPi1, float);
DECLARE_SOA_COLUMN(NSigTpcKa1, nSigTpcKa1, float);
DECLARE_SOA_COLUMN(NSigTofPi1, nSigTofPi1, float);
DECLARE_SOA_COLUMN(NSigTofKa1, nSigTofKa1, float);
DECLARE_SOA_COLUMN(DecayLength, decayLength, float);
DECLARE_SOA_COLUMN(DecayLengthXY, decayLengthXY, float);
DECLARE_SOA_COLUMN(DecayLengthNormalised, decayLengthNormalised, float);
DECLARE_SOA_COLUMN(DecayLengthXYNormalised, decayLengthXYNormalised, float);
DECLARE_SOA_COLUMN(Cpa, cpa, float);
DECLARE_SOA_COLUMN(CpaXY, cpaXY, float);
DECLARE_SOA_COLUMN(MaxNormalisedDeltaIP, maxNormalisedDeltaIP, float);
DECLARE_SOA_COLUMN(Ct, ct, float);
DECLARE_SOA_COLUMN(ImpactParameterProduct, impactParameterProduct, float);
DECLARE_SOA_COLUMN(CosThetaStar, cosThetaStar, float);
DECLARE_SOA_COLUMN(FlagMc, flagMc, int8_t);
DECLARE_SOA_COLUMN(OriginMcRec, originMcRec, int8_t); // is prompt or non-prompt, reco level
DECLARE_SOA_COLUMN(OriginMcGen, originMcGen, int8_t); // is prompt or non-prompt, Gen level
DECLARE_SOA_INDEX_COLUMN_FULL(Candidate, candidate, int, HfCand2Prong, "_0");
// Events
DECLARE_SOA_COLUMN(IsEventReject, isEventReject, int);
DECLARE_SOA_COLUMN(RunNumber, runNumber, int);
} // namespace full

DECLARE_SOA_TABLE(HfCandD0Lites, "AOD", "HFCANDD0LITE",
                  hf_cand::Chi2PCA,
                  full::DecayLength,
                  full::DecayLengthXY,
                  full::DecayLengthNormalised,
                  full::DecayLengthXYNormalised,
                  full::PtProng0,
                  full::PtProng1,
                  hf_cand::ImpactParameter0,
                  hf_cand::ImpactParameter1,
                  full::ImpactParameterNormalised0,
                  full::ImpactParameterNormalised1,
                  full::NSigTpcPi0,
                  full::NSigTpcKa0,
                  full::NSigTofPi0,
                  full::NSigTofKa0,
                  full::NSigTpcPi1,
                  full::NSigTpcKa1,
                  full::NSigTofPi1,
                  full::NSigTofKa1,
                  full::CandidateSelFlag,
                  full::M,
                  full::Pt,
                  full::Cpa,
                  full::CpaXY,
                  full::MaxNormalisedDeltaIP,
                  full::ImpactParameterProduct,
                  full::Eta,
                  full::Phi,
                  full::Y,
                  full::FlagMc,
                  full::OriginMcRec)

DECLARE_SOA_TABLE(HfCandD0Fulls, "AOD", "HFCANDD0FULL",
                  full::CollisionId,
                  collision::PosX,
                  collision::PosY,
                  collision::PosZ,
                  hf_cand::XSecondaryVertex,
                  hf_cand::YSecondaryVertex,
                  hf_cand::ZSecondaryVertex,
                  hf_cand::ErrorDecayLength,
                  hf_cand::ErrorDecayLengthXY,
                  hf_cand::Chi2PCA,
                  full::RSecondaryVertex,
                  full::DecayLength,
                  full::DecayLengthXY,
                  full::DecayLengthNormalised,
                  full::DecayLengthXYNormalised,
                  full::ImpactParameterNormalised0,
                  full::PtProng0,
                  full::PProng0,
                  full::ImpactParameterNormalised1,
                  full::PtProng1,
                  full::PProng1,
                  hf_cand::PxProng0,
                  hf_cand::PyProng0,
                  hf_cand::PzProng0,
                  hf_cand::PxProng1,
                  hf_cand::PyProng1,
                  hf_cand::PzProng1,
                  hf_cand::ImpactParameter0,
                  hf_cand::ImpactParameter1,
                  hf_cand::ErrorImpactParameter0,
                  hf_cand::ErrorImpactParameter1,
                  full::NSigTpcPi0,
                  full::NSigTpcKa0,
                  full::NSigTofPi0,
                  full::NSigTofKa0,
                  full::NSigTpcPi1,
                  full::NSigTpcKa1,
                  full::NSigTofPi1,
                  full::NSigTofKa1,
                  full::CandidateSelFlag,
                  full::M,
                  full::MaxNormalisedDeltaIP,
                  full::ImpactParameterProduct,
                  full::CosThetaStar,
                  full::Pt,
                  full::P,
                  full::Cpa,
                  full::CpaXY,
                  full::Ct,
                  full::Eta,
                  full::Phi,
                  full::Y,
                  full::E,
                  full::FlagMc,
                  full::OriginMcRec,
                  full::CandidateId);

DECLARE_SOA_TABLE(HfCandD0FullEvs, "AOD", "HFCANDD0FULLEV",
                  full::CollisionId,
                  collision::NumContrib,
                  collision::PosX,
                  collision::PosY,
                  collision::PosZ,
                  full::IsEventReject,
                  full::RunNumber);

DECLARE_SOA_TABLE(HfCandD0FullPs, "AOD", "HFCANDD0FULLP",
                  full::CollisionId,
                  full::Pt,
                  full::Eta,
                  full::Phi,
                  full::Y,
                  full::FlagMc,
                  full::OriginMcGen,
                  full::CandidateId);

} // namespace o2::aod

/// Writes the full information in an output TTree
struct HfTreeCreatorD0ToKPi {
  Produces<o2::aod::HfCandD0Fulls> rowCandidateFull;
  Produces<o2::aod::HfCandD0FullEvs> rowCandidateFullEvents;
  Produces<o2::aod::HfCandD0FullPs> rowCandidateFullParticles;
  Produces<o2::aod::HfCandD0Lites> rowCandidateLite;

  Configurable<bool> fillCandidateLiteTable{"fillCandidateLiteTable", false, "Switch to fill lite table with candidate properties"};
  // parameters for production of training samples
  Configurable<bool> fillOnlySignal{"fillOnlySignal", false, "Flag to fill derived tables with signal for ML trainings"};
  Configurable<bool> fillOnlyBackground{"fillOnlyBackground", false, "Flag to fill derived tables with background for ML trainings"};
  Configurable<float> downSampleBkgFactor{"downSampleBkgFactor", 1., "Fraction of background candidates to keep for ML trainings"};
  Configurable<float> ptMaxForDownSample{"ptMaxForDownSample", 10., "Maximum pt for the application of the downsampling factor"};

  using SelectedCandidatesMc = soa::Filtered<soa::Join<aod::HfCand2Prong, aod::HfCand2ProngMcRec, aod::HfSelD0>>;
  using MatchedGenCandidatesMc = soa::Filtered<soa::Join<aod::McParticles, aod::HfCand2ProngMcGen>>;

  Filter filterSelectCandidates = aod::hf_sel_candidate_d0::isSelD0 >= 1 || aod::hf_sel_candidate_d0::isSelD0bar >= 1;
  Filter filterMcGenMatching = nabs(o2::aod::hf_cand_2prong::flagMcMatchGen) == static_cast<int8_t>(BIT(DecayType::D0ToPiK));

  Partition<SelectedCandidatesMc> reconstructedCandSig = nabs(aod::hf_cand_2prong::flagMcMatchRec) == static_cast<int8_t>(BIT(DecayType::D0ToPiK));
  Partition<SelectedCandidatesMc> reconstructedCandBkg = nabs(aod::hf_cand_2prong::flagMcMatchRec) != static_cast<int8_t>(BIT(DecayType::D0ToPiK));

  void init(InitContext const&)
  {
  }

  template <typename T>
  void fillEvent(const T& collision, int isEventReject, int runNumber)
  {
    rowCandidateFullEvents(
      collision.globalIndex(),
      collision.numContrib(),
      collision.posX(),
      collision.posY(),
      collision.posZ(),
      isEventReject,
      runNumber);
  }

  template <typename T, typename U>
  auto fillTable(const T& candidate, const U& prong0, const U& prong1, int candFlag, double invMass, double cosThetaStar,
                 double ct, double y, double e, int8_t flagMc, int8_t origin)
  {
    if (fillCandidateLiteTable) {
      rowCandidateLite(
        candidate.chi2PCA(),
        candidate.decayLength(),
        candidate.decayLengthXY(),
        candidate.decayLengthNormalised(),
        candidate.decayLengthXYNormalised(),
        candidate.ptProng0(),
        candidate.ptProng1(),
        candidate.impactParameterNormalised0(),
        candidate.impactParameterNormalised1(),
        candidate.impactParameter0(),
        candidate.impactParameter1(),
        prong0.tpcNSigmaPi(),
        prong0.tpcNSigmaKa(),
        prong0.tofNSigmaPi(),
        prong0.tofNSigmaKa(),
        prong1.tpcNSigmaPi(),
        prong1.tpcNSigmaKa(),
        prong1.tofNSigmaPi(),
        prong1.tofNSigmaKa(),
        1 << candFlag,
        invMass,
        candidate.pt(),
        candidate.cpa(),
        candidate.cpaXY(),
        candidate.maxNormalisedDeltaIP(),
        candidate.impactParameterProduct(),
        candidate.eta(),
        candidate.phi(),
        y,
        flagMc,
        origin);
    } else {
      rowCandidateFull(
        candidate.collisionId(),
        candidate.posX(),
        candidate.posY(),
        candidate.posZ(),
        candidate.xSecondaryVertex(),
        candidate.ySecondaryVertex(),
        candidate.zSecondaryVertex(),
        candidate.errorDecayLength(),
        candidate.errorDecayLengthXY(),
        candidate.chi2PCA(),
        candidate.rSecondaryVertex(),
        candidate.decayLength(),
        candidate.decayLengthXY(),
        candidate.decayLengthNormalised(),
        candidate.decayLengthXYNormalised(),
        candidate.impactParameterNormalised0(),
        candidate.ptProng0(),
        RecoDecay::p(candidate.pxProng0(), candidate.pyProng0(), candidate.pzProng0()),
        candidate.impactParameterNormalised1(),
        candidate.ptProng1(),
        RecoDecay::p(candidate.pxProng1(), candidate.pyProng1(), candidate.pzProng1()),
        candidate.pxProng0(),
        candidate.pyProng0(),
        candidate.pzProng0(),
        candidate.pxProng1(),
        candidate.pyProng1(),
        candidate.pzProng1(),
        candidate.impactParameter0(),
        candidate.impactParameter1(),
        candidate.errorImpactParameter0(),
        candidate.errorImpactParameter1(),
        prong0.tpcNSigmaPi(),
        prong0.tpcNSigmaKa(),
        prong0.tofNSigmaPi(),
        prong0.tofNSigmaKa(),
        prong1.tpcNSigmaPi(),
        prong1.tpcNSigmaKa(),
        prong1.tofNSigmaPi(),
        prong1.tofNSigmaKa(),
        1 << candFlag,
        invMass,
        candidate.maxNormalisedDeltaIP(),
        candidate.impactParameterProduct(),
        cosThetaStar,
        candidate.pt(),
        candidate.p(),
        candidate.cpa(),
        candidate.cpaXY(),
        ct,
        candidate.eta(),
        candidate.phi(),
        y,
        e,
        flagMc,
        origin,
        candidate.globalIndex());
    }
  }

  void processData(aod::Collisions const& collisions,
                   soa::Filtered<soa::Join<aod::HfCand2Prong, aod::HfSelD0>> const& candidates,
                   aod::BigTracksPID const&, aod::BCs const&)
  {
    // Filling event properties
    rowCandidateFullEvents.reserve(collisions.size());
    for (auto const& collision : collisions) {
      fillEvent(collision, 0, collision.bc().runNumber());
    }

    // Filling candidate properties
    if (fillCandidateLiteTable) {
      rowCandidateLite.reserve(candidates.size());
    } else {
      rowCandidateFull.reserve(candidates.size());
    }
    for (auto const& candidate : candidates) {
      if (downSampleBkgFactor < 1.) {
        float pseudoRndm = candidate.ptProng0() * 1000. - (int64_t)(candidate.ptProng0() * 1000);
        if (candidate.pt() < ptMaxForDownSample && pseudoRndm >= downSampleBkgFactor) {
          continue;
        }
      }
      auto prong0 = candidate.prong0_as<aod::BigTracksPID>();
      auto prong1 = candidate.prong1_as<aod::BigTracksPID>();
      double yD = yD0(candidate);
      double eD = eD0(candidate);
      double ctD = ctD0(candidate);
      if (candidate.isSelD0()) {
        fillTable(candidate, prong0, prong1, 0, invMassD0ToPiK(candidate), cosThetaStarD0(candidate), ctD, yD, eD, 0, 0);
      }
      if (candidate.isSelD0bar()) {
        fillTable(candidate, prong0, prong1, 1, invMassD0barToKPi(candidate), cosThetaStarD0bar(candidate), ctD, yD, eD, 0, 0);
      }
    }
  }

  PROCESS_SWITCH(HfTreeCreatorD0ToKPi, processData, "Process data", true);

  void processMc(aod::Collisions const& collisions,
                 aod::McCollisions const&,
                 SelectedCandidatesMc const& candidates,
                 MatchedGenCandidatesMc const& particles,
                 aod::BigTracksPID const&, aod::BCs const&)
  {
    // Filling event properties
    rowCandidateFullEvents.reserve(collisions.size());
    for (auto const& collision : collisions) {
      fillEvent(collision, 0, collision.bc().runNumber());
    }

    // Filling candidate properties
    if (fillOnlySignal) {
      if (fillCandidateLiteTable) {
        rowCandidateLite.reserve(reconstructedCandSig.size());
      } else {
        rowCandidateFull.reserve(reconstructedCandSig.size());
      }
      for (const auto& candidate : reconstructedCandSig) {
        auto prong0 = candidate.prong0_as<aod::BigTracksPID>();
        auto prong1 = candidate.prong0_as<aod::BigTracksPID>();
        double yD = yD0(candidate);
        double eD = eD0(candidate);
        double ctD = ctD0(candidate);
        if (candidate.isSelD0()) {
          fillTable(candidate, prong0, prong1, 0, invMassD0ToPiK(candidate), cosThetaStarD0(candidate), ctD, yD, eD, candidate.flagMcMatchRec(), candidate.originMcRec());
        }
        if (candidate.isSelD0bar()) {
          fillTable(candidate, prong0, prong1, 1, invMassD0barToKPi(candidate), cosThetaStarD0bar(candidate), ctD, yD, eD, candidate.flagMcMatchRec(), candidate.originMcRec());
        }
      }
    } else if (fillOnlyBackground) {
      if (fillCandidateLiteTable) {
        rowCandidateLite.reserve(reconstructedCandBkg.size());
      } else {
        rowCandidateFull.reserve(reconstructedCandBkg.size());
      }
      for (const auto& candidate : reconstructedCandBkg) {
        if (downSampleBkgFactor < 1.) {
          float pseudoRndm = candidate.ptProng0() * 1000. - (int64_t)(candidate.ptProng0() * 1000);
          if (candidate.pt() < ptMaxForDownSample && pseudoRndm >= downSampleBkgFactor) {
            continue;
          }
        }
        auto prong0 = candidate.prong0_as<aod::BigTracksPID>();
        auto prong1 = candidate.prong0_as<aod::BigTracksPID>();
        double yD = yD0(candidate);
        double eD = eD0(candidate);
        double ctD = ctD0(candidate);
        if (candidate.isSelD0()) {
          fillTable(candidate, prong0, prong1, 0, invMassD0ToPiK(candidate), cosThetaStarD0(candidate), ctD, yD, eD, candidate.flagMcMatchRec(), candidate.originMcRec());
        }
        if (candidate.isSelD0bar()) {
          fillTable(candidate, prong0, prong1, 1, invMassD0barToKPi(candidate), cosThetaStarD0bar(candidate), ctD, yD, eD, candidate.flagMcMatchRec(), candidate.originMcRec());
        }
      }
    } else {
      if (fillCandidateLiteTable) {
        rowCandidateLite.reserve(candidates.size());
      } else {
        rowCandidateFull.reserve(candidates.size());
      }
      for (const auto& candidate : candidates) {
        auto prong0 = candidate.prong0_as<aod::BigTracksPID>();
        auto prong1 = candidate.prong0_as<aod::BigTracksPID>();
        double yD = yD0(candidate);
        double eD = eD0(candidate);
        double ctD = ctD0(candidate);
        if (candidate.isSelD0()) {
          fillTable(candidate, prong0, prong1, 0, invMassD0ToPiK(candidate), cosThetaStarD0(candidate), ctD, yD, eD, candidate.flagMcMatchRec(), candidate.originMcRec());
        }
        if (candidate.isSelD0bar()) {
          fillTable(candidate, prong0, prong1, 1, invMassD0barToKPi(candidate), cosThetaStarD0bar(candidate), ctD, yD, eD, candidate.flagMcMatchRec(), candidate.originMcRec());
        }
      }
    }

    // Filling particle properties
    rowCandidateFullParticles.reserve(particles.size());
    for (auto const& particle : particles) {
      if (std::abs(particle.flagMcMatchGen()) == 1 << DecayType::D0ToPiK) {
        rowCandidateFullParticles(
          particle.mcCollisionId(),
          particle.pt(),
          particle.eta(),
          particle.phi(),
          RecoDecay::y(array{particle.px(), particle.py(), particle.pz()}, RecoDecay::getMassPDG(particle.pdgCode())),
          particle.flagMcMatchGen(),
          particle.originMcGen(),
          particle.globalIndex());
      }
    }
  }

  PROCESS_SWITCH(HfTreeCreatorD0ToKPi, processMc, "Process MC", false);
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  WorkflowSpec workflow;
  workflow.push_back(adaptAnalysisTask<HfTreeCreatorD0ToKPi>(cfgc));
  return workflow;
}
