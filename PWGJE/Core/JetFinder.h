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

// jet finder task
//
// Authors: Nima Zardoshti, Jochen Klein

#ifndef PWGJE_CORE_JETFINDER_H_
#define PWGJE_CORE_JETFINDER_H_

#include <memory>
#include <vector>

#include <TDatabasePDG.h>
#include <TPDGCode.h>
#include <TMath.h>

#include "fastjet/PseudoJet.hh"
#include "fastjet/ClusterSequenceArea.hh"
#include "fastjet/AreaDefinition.hh"
#include "fastjet/JetDefinition.hh"
#include "fastjet/tools/JetMedianBackgroundEstimator.hh"
#include "fastjet/tools/Subtractor.hh"
#include "fastjet/contrib/ConstituentSubtractor.hh"

enum class JetType {
  full = 0,
  charged = 1,
  neutral = 2,
};

class JetFinder
{

 public:
  enum class BkgSubMode { none,
                          rhoAreaSub,
                          constSub };
  BkgSubMode bkgSubMode;

  void setBkgSubMode(BkgSubMode bSM) { bkgSubMode = bSM; }

  /// Performs jet finding
  /// \note the input particle and jet lists are passed by reference
  /// \param inputParticles vector of input particles/tracks
  /// \param jets veector of jets to be filled
  /// \return ClusterSequenceArea object needed to access constituents
  // fastjet::ClusterSequenceArea findJets(std::vector<fastjet::PseudoJet> &inputParticles, std::vector<fastjet::PseudoJet> &jets);

  static constexpr float mPion = 0.139; // TDatabasePDG::Instance()->GetParticle(211)->Mass(); //can be removed when pion mass becomes default for unidentified tracks

  float phiMin;
  float phiMax;
  float etaMin;
  float etaMax;

  float jetR;
  float jetPtMin;
  float jetPtMax;
  float jetPhiMin;
  float jetPhiMax;
  float jetEtaMin;
  float jetEtaMax;

  float ghostEtaMin;
  float ghostEtaMax;
  float ghostArea;
  int ghostRepeatN;
  double ghostktMean;
  float gridScatter;
  float ktScatter;

  float jetBkgR;
  float bkgPhiMin;
  float bkgPhiMax;
  float bkgEtaMin;
  float bkgEtaMax;

  float constSubAlpha;
  float constSubRMax;

  bool isReclustering;
  bool isTriggering;

  fastjet::JetAlgorithm algorithm;
  fastjet::RecombinationScheme recombScheme;
  fastjet::Strategy strategy;
  fastjet::AreaType areaType;
  fastjet::GhostedAreaSpec ghostAreaSpec;
  fastjet::JetDefinition jetDef;
  fastjet::AreaDefinition areaDef;
  fastjet::Selector selJets;
  fastjet::Selector selGhosts;

  fastjet::JetAlgorithm algorithmBkg;
  fastjet::RecombinationScheme recombSchemeBkg;
  fastjet::Strategy strategyBkg;
  fastjet::AreaType areaTypeBkg;
  fastjet::JetDefinition jetDefBkg;
  fastjet::AreaDefinition areaDefBkg;
  fastjet::Selector selRho;

  /// Default constructor
  explicit JetFinder(float eta_Min = -0.9, float eta_Max = 0.9, float phi_Min = 0.0, float phi_Max = 2 * M_PI) : bkgSubMode(BkgSubMode::none),
                                                                                                                 phiMin(phi_Min),
                                                                                                                 phiMax(phi_Max),
                                                                                                                 etaMin(eta_Min),
                                                                                                                 etaMax(eta_Max),
                                                                                                                 jetR(0.4),
                                                                                                                 jetPtMin(0.0),
                                                                                                                 jetPtMax(1000.0),
                                                                                                                 jetPhiMin(phi_Min),
                                                                                                                 jetPhiMax(phi_Max),
                                                                                                                 jetEtaMin(-99.0),
                                                                                                                 jetEtaMax(99.0),
                                                                                                                 ghostEtaMin(eta_Min),
                                                                                                                 ghostEtaMax(eta_Max),
                                                                                                                 ghostArea(0.005),
                                                                                                                 ghostRepeatN(1),
                                                                                                                 ghostktMean(1e-100), // is float precise enough?
                                                                                                                 gridScatter(1.0),
                                                                                                                 ktScatter(0.1),
                                                                                                                 jetBkgR(0.2),
                                                                                                                 bkgPhiMin(phi_Min),
                                                                                                                 bkgPhiMax(phi_Max),
                                                                                                                 bkgEtaMin(eta_Min),
                                                                                                                 bkgEtaMax(eta_Max),
                                                                                                                 constSubAlpha(1.0),
                                                                                                                 constSubRMax(0.6),
                                                                                                                 isReclustering(false),
                                                                                                                 isTriggering(false),
                                                                                                                 algorithm(fastjet::antikt_algorithm),
                                                                                                                 recombScheme(fastjet::E_scheme),
                                                                                                                 strategy(fastjet::Best),
                                                                                                                 areaType(fastjet::active_area),
                                                                                                                 algorithmBkg(fastjet::JetAlgorithm(fastjet::kt_algorithm)),
                                                                                                                 recombSchemeBkg(fastjet::RecombinationScheme(fastjet::E_scheme)),
                                                                                                                 strategyBkg(fastjet::Best),
                                                                                                                 areaTypeBkg(fastjet::active_area)
  {

    // default constructor
  }

  /// Default destructor
  ~JetFinder() = default;

  /// Sets the jet finding parameters
  void setParams();

  /// Sets the background subtraction estimater pointer
  void setBkgE();

  /// Sets the background subtraction pointer
  void setSub();

  /// Performs jet finding
  /// \note the input particle and jet lists are passed by reference
  /// \param inputParticles vector of input particles/tracks
  /// \param jets veector of jets to be filled
  /// \return ClusterSequenceArea object needed to access constituents
  fastjet::ClusterSequenceArea findJets(std::vector<fastjet::PseudoJet>& inputParticles, std::vector<fastjet::PseudoJet>& jets); // ideally find a way of passing the cluster sequence as a reeference

 private:
  // void setParams();
  // void setBkgSub();
  std::unique_ptr<fastjet::BackgroundEstimatorBase> bkgE;
  std::unique_ptr<fastjet::Subtractor> sub;
  std::unique_ptr<fastjet::contrib::ConstituentSubtractor> constituentSub;

  ClassDefNV(JetFinder, 1);
};

#endif // PWGJE_CORE_JETFINDER_H_
