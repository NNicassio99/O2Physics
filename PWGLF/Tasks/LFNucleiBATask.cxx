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

///
/// \file LFNucleiBATask.cxx
///
/// \brief  Analysis task for the measurement of the coalescence parameter B2/B3 in pp collisions for (anti)deutheron/(anti)helium-3
///
/// \author Giovanni Malfattore <giovanni.malfattore@cern.ch> and Rutuparna Rath <rutuparna.rath@cern.ch>
///
#include "PWGLF/DataModel/LFNucleiTables.h"
#include <TLorentzVector.h>
#include "ReconstructionDataFormats/Track.h"
#include "Framework/runDataProcessing.h"
#include "Framework/AnalysisTask.h"
#include "Framework/AnalysisDataModel.h"
#include "Framework/ASoAHelpers.h"
#include "Framework/HistogramRegistry.h"

#include "Common/DataModel/Multiplicity.h"
#include "Common/DataModel/PIDResponse.h"
#include "Common/DataModel/EventSelection.h"
#include "Common/DataModel/Centrality.h"
#include "Common/DataModel/TrackSelectionTables.h"
#include "Common/Core/trackUtilities.h"
#include "PWGLF/DataModel/LFParticleIdentification.h"
#include "ReconstructionDataFormats/PID.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::framework::expressions;

struct LFNucleiBATask {
  HistogramRegistry histos{"Histos", {}, OutputObjHandlingPolicy::AnalysisObject};
  HistogramRegistry spectraGen{"spectraGen", {}, OutputObjHandlingPolicy::AnalysisObject, false, true};

  Configurable<bool> enablePr{"enablePr", true, "Flag to enable proton analysis."};
  Configurable<bool> enableDe{"enableDe", true, "Flag to enable deuteron analysis."};
  Configurable<bool> enableTr{"enableTr", true, "Flag to enable triton analysis."};
  Configurable<bool> enableHe{"enableHe", true, "Flag to enable helium-3 analysis."};
  Configurable<bool> enableAl{"enableAl", true, "Flag to enable alpha analysis."};

  Configurable<float> cfgHighCutVertex{"cfgHighCutVertex", 10.0f, "Accepted z-vertex upper limit"};
  Configurable<float> cfgLowCutVertex{"cfgLowCutVertex", -10.0f, "Accepted z-vertex lower limit"};
  Configurable<float> cfgCutITSClusters{"cfgCutITSClusters", -1.f, "Minimum number of ITS clusters"};
  Configurable<float> cfgCutTPCXRows{"cfgCutTPCXRows", -1.f, "Minimum number of crossed TPC rows"};
  Configurable<float> cfgCutTPCClusters{"cfgCutTPCClusters", -1.f, "Minimum number of found TPC clusters"};

  Configurable<float> etaCut{"etaCut", 0.8f, "Value of the eta selection for spectra (default 0.8)"};
  Configurable<float> yCut{"yCut", 1.0f, "Value of the rapidity selection for spectra (default 1.0)"};
  Configurable<float> nsigmaTPCcut{"nsigmaTPCcut", 7.f, "Value of the Nsigma TPC cut"};
  Configurable<float> nsigmaTOFcut{"nsigmaTOFcut", 5.f, "Value of the Nsigma TOF cut"};
  Configurable<float> nsigmaTPCPr{"nsigmaTPCPr", 3.f, "Value of the Nsigma TPC cut for protons"};
  Configurable<float> nsigmaTPCDe{"nsigmaTPCDe", 3.f, "Value of the Nsigma TPC cut for deuterons"};
  Configurable<float> nsigmaTPCTr{"nsigmaTPCTr", 3.f, "Value of the Nsigma TPC cut for tritons"};
  Configurable<float> nsigmaTPCHe{"nsigmaTPCHe", 3.f, "Value of the Nsigma TPC cut for helium-3"};
  Configurable<float> nsigmaTPCAl{"nsigmaTPCAl", 3.f, "Value of the Nsigma TPC cut for alpha"};
  Configurable<float> ProtonNsigmaTPCcutInTOFbeta{"ProtonNsigmaTPCcutInTOFbeta", 3.5f, "Value of the Nsigma TPC cut (proton) for TOF beta distribution"};
  Configurable<float> nsigmaTPCStrongCut{"nsigmaTPCStrongCut", 3.f, "Value of the Nsigma TPC (Strong) proton cut, if enabled"};
  Configurable<float> betaCut{"betaCut", 0.4f, "Value of the beta selection for TOF cut (default 0.4)"};

  ConfigurableAxis binsPt{"binsPt", {VARIABLE_WIDTH, 0.0, 0.1, 0.12, 0.14, 0.16, 0.18, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5, 0.55, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 2.0, 2.2, 2.4, 2.6, 2.8, 3.0, 3.2, 3.4, 3.6, 3.8, 4.0, 4.2, 4.4, 4.6, 4.8, 5.0}, ""};
  ConfigurableAxis binsdEdx{"binsdEdx", {1000, 0.f, 1000.f}, ""};
  ConfigurableAxis binsBeta{"binsBeta", {120, 0.0, 1.2}, ""};
  ConfigurableAxis binsDCA{"binsDCA", {400, -1.f, 1.f}, ""};
  ConfigurableAxis binsSigmaTPC{"binsSigmaTPC", {1000, -100, 100}, ""};
  ConfigurableAxis binsSigmaTOF{"binsSigmaTOF", {1000, -100, 100}, ""};
  ConfigurableAxis binsMassPr{"binsMassPr", {100, -1., 1.f}, ""};
  ConfigurableAxis binsMassDe{"binsMassDe", {180, -1.8, 1.8f}, ""};
  ConfigurableAxis binsMassTr{"binsMassTr", {250, -2.5, 2.5f}, ""};
  ConfigurableAxis binsMassHe{"binsMassHe", {300, -3., 3.f}, ""};

  Configurable<bool> enableFiltering{"enableFiltering", false, "Flag to enable filtering for p,d,t,He only"};
  Configurable<bool> doTPCcalib{"doTPCcalib", false, "Flag to stop task after dEdX. Please set to 'processData'."};
  Configurable<bool> doTOFplots{"doTOFplots", true, "Flag to export plots of tracks with 1 hit on TOF."};
  Configurable<bool> enableExpSignalTPC{"enableExpSignalTPC", true, "Flag to export dEdX - dEdX(exp) plots."};
  Configurable<bool> enableExpSignalTOF{"enableExpSignalTOF", false, "Flag to export T - T(exp) plots."};
  Configurable<bool> isPVContributorCut{"isPVContributorCut", false, "Flag to enable isPVContributor cut."};

  Configurable<bool> makeDCABeforeCutPlots{"makeDCABeforeCutPlots", false, "Flag to enable plots of DCA before cuts"};
  Configurable<bool> makeDCAAfterCutPlots{"makeDCAAfterCutPlots", false, "Flag to enable plots of DCA after cuts"};
  Configurable<bool> enableDCACustomCut{"enableDCACustomCut", false, "Flag to enable DCA custom cuts - unflag to use standard isGlobalCut DCA cut"};
  Configurable<float> DCAxyCustomCut{"DCAxyCustomCut", 2.0f, "Value of the DCAxy selection for spectra (default 2.0 cm)"};
  Configurable<float> DCAzCustomCut{"DCAzCustomCut", 2.0f, "Value of the DCAz selection for spectra (default 2.0 cm)"};

  Configurable<bool> enableEvTimeSplitting{"enableEvTimeSplitting", true, "Flag to enable histograms splitting depending on the Event Time used"};
  Configurable<bool> enablePIDplot{"enablePIDplot", false, "Flag to enable PID histograms for debug"};
  Configurable<bool> enableBetaCut{"enableBetaCut", false, "Flag to enable TOF histograms with beta cut for debug"};

  Configurable<bool> enableDebug{"enableDebug", false, "Flag to enable histograms for debug"};
  Configurable<bool> enableStrongCut{"enableStrongCut", false, "Flag to change | NSigma TPC(nucl)| < nSigmaTPC --> NOT | NSigma TPC(p)| > nStrongCut"};
  Configurable<bool> enableNucleiHardCut{"enableNucleiHardCut", false, "Flag to enable TPC sigma histograms filled without the 'nearby' particle (at low momentum)"};
  Configurable<bool> enablePtSpectra{"enablePtSpectra", false, "Flag to enable histograms for efficiency debug."};
  Configurable<bool> rapDebug{"rapDebug", false, "Flag to enable helium rapidity cut debug histograms."};

  Configurable<int> useHasTRDConfig{"useHasTRDConfig", 0, "No selections on TRD (0); With TRD (1); Without TRD (2)"};
  Configurable<int> massTOFConfig{"massTOFConfig", 0, "Estimate massTOF using beta with (0) TPC momentum (1) TOF expected momentum"};
  Configurable<int> tritonSelConfig{"tritonSelConfig", 0, "Select tritons using (0) 3Sigma TPC triton (1) additional 3sigma TPC pi,K,p veto cut"};
  Configurable<int> helium3Pt{"helium3Pt", 0, "Select use default pT (0) or use instead 2*pT (1) for helium-3"};

  Configurable<int> nITSLayer{"nITSLayer", 0, "ITS Layer (0-6)"};
  Configurable<bool> usenITSLayer{"usenITSLayer", false, "Flag to enable ITS layer hit"};

  static constexpr int PDGPion = 211;
  static constexpr int PDGKaon = 321;
  static constexpr int PDGProton = 2212;
  static constexpr int PDGDeuteron = 1000010020;
  static constexpr int PDGTriton = 1000010030;
  static constexpr int PDGHelium = 1000020030;
  static constexpr int PDGAlpha = 1000020040;
  static constexpr float fMassProton = 0.938272088f;
  static constexpr float fMassDeuteron = 1.87561f;
  static constexpr float fMassTriton = 2.80892f;
  static constexpr float fMassHelium = 2.80839f;
  static constexpr float fMassAlpha = 3.72738f;

  void init(o2::framework::InitContext&)
  {
    const AxisSpec pAxis{binsPt, "#it{p} (GeV/#it{c})"};
    const AxisSpec ptAxis{binsPt, "#it{p}_{T} (GeV/#it{c})"};
    const AxisSpec dedxAxis{binsdEdx, "d#it{E}/d#it{x} A.U."};
    const AxisSpec betaAxis{binsBeta, "TOF #beta"};
    const AxisSpec dcaxyAxis{binsDCA, "DCAxy (cm)"};
    const AxisSpec dcazAxis{binsDCA, "DCAz (cm)"};
    const AxisSpec massPrAxis{binsMassPr, ""};
    const AxisSpec massDeAxis{binsMassDe, ""};
    const AxisSpec massTrAxis{binsMassTr, ""};
    const AxisSpec massHeAxis{binsMassHe, ""};
    const AxisSpec SigmaTPCAxis{binsSigmaTPC, ""};
    const AxisSpec SigmaTOFAxis{binsSigmaTOF, ""};

    if (doprocessData == true && doprocessMCReco == true) {
      LOG(fatal) << "Can't enable processData and processMCReco in the same time, pick one!";
    }

    histos.add<TH1>("event/h1VtxZ", "V_{z};V_{z} (in cm); counts", HistType::kTH1F, {{1500, -15, 15}});
    histos.add<TH1>("tracks/h1pT", "Track #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); counts", HistType::kTH1F, {{500, 0., 10.}});
    histos.add<TH1>("tracks/h1p", "Track momentum; p (GeV/#it{c}); counts", HistType::kTH1F, {{500, 0., 10.}});

    histos.add<TH1>("qa/h1ITSncr", "number of crossed rows in ITS; ITSncr; counts", HistType::kTH1F, {{12, 0, 12}});
    histos.add<TH1>("qa/h1TPCncr", "number of crossed rows in TPC; TPCncr; counts", HistType::kTH1F, {{150, 60, 170}});
    histos.add<TH1>("qa/h1rTPC", "ratio of ncr over findable in TPC; rTPC; counts", HistType::kTH1F, {{200, 0.9, 1.8}});
    histos.add<TH1>("qa/h1TPCnfound", "ratio of found cluster in TPC; TPCnfound; counts", HistType::kTH1F, {{150, 60, 170}});
    histos.add<TH1>("qa/h1chi2ITS", "#chi^{2}_{ITS}/n_{ITS}; #chi^{2}_{ITS}/n_{ITS};counts", HistType::kTH1F, {{51, -0.5, 50.5}});
    histos.add<TH1>("qa/h1chi2TPC", "#chi^{2}_{TPC}/n_{TPC}; #chi^{2}_{TPC}/n_{TPC}; counts", HistType::kTH1F, {{11, -0.5, 10.5}});

    if (enablePtSpectra) {
      histos.add<TH2>("tracks/eff/h2pVsTPCmomentum", "#it{p}_{TPC} vs #it{p}; #it{p}_{TPC}; #it{p}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
      if (doTOFplots)
        histos.add<TH2>("tracks/eff/h2TPCmomentumVsTOFExpMomentum", "#it{p}_{TOF} vs #it{p}_{TPC}; #it{p}_{TOF}; #it{p}_{TPC}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});

      if (enablePr) {
        histos.add<TH2>("tracks/eff/proton/h2pVsTPCmomentumPr", "#it{p}_{TPC} vs #it{p}; #it{p}_{TPC}; #it{p}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
        histos.add<TH2>("tracks/eff/proton/h2pVsTPCmomentumantiPr", "#it{p}_{TPC} vs #it{p}; #it{p}_{TPC}; #it{p}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
        if (doTOFplots) {
          histos.add<TH2>("tracks/eff/proton/h2pVsTOFExpMomentumPr", "#it{p}_{TOF} vs #it{p}; #it{p}_{TOF}; #it{p}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
          histos.add<TH2>("tracks/eff/proton/h2pVsTOFExpMomentumantiPr", "#it{p}_{TOF} vs #it{p}; #it{p}_{TOF}; #it{p}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
          histos.add<TH2>("tracks/eff/proton/h2TPCmomentumVsTOFExpMomentumPr", "#it{p}_{TOF} vs #it{p}_{TPC}; #it{p}_{TOF}; #it{p}_{TPC}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
          histos.add<TH2>("tracks/eff/proton/h2TPCmomentumVsTOFExpMomentumantiPr", "#it{p}_{TOF} vs #it{p}_{TPC}; #it{p}_{TOF}; #it{p}_{TPC}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
        }
      }
      if (enableDe) {
        histos.add<TH2>("tracks/eff/deuteron/h2pVsTPCmomentumDe", "#it{p}_{TPC} vs #it{p}; #it{p}_{TPC}; #it{p}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
        histos.add<TH2>("tracks/eff/deuteron/h2pVsTPCmomentumantiDe", "#it{p}_{TPC} vs #it{p}; #it{p}_{TPC}; #it{p}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
        if (doTOFplots) {
          histos.add<TH2>("tracks/eff/deuteron/h2pVsTOFExpMomentumDe", "#it{p}_{TOF} vs #it{p}; #it{p}_{TOF}; #it{p}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
          histos.add<TH2>("tracks/eff/deuteron/h2pVsTOFExpMomentumantiDe", "#it{p}_{TOF} vs #it{p}; #it{p}_{TOF}; #it{p}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
          histos.add<TH2>("tracks/eff/deuteron/h2TPCmomentumVsTOFExpMomentumDe", "#it{p}_{TOF} vs #it{p}_{TPC}; #it{p}_{TOF}; #it{p}_{TPC}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
          histos.add<TH2>("tracks/eff/deuteron/h2TPCmomentumVsTOFExpMomentumantiDe", "#it{p}_{TOF} vs #it{p}_{TPC}; #it{p}_{TOF}; #it{p}_{TPC}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
        }
      }
      if (enableTr) {
        histos.add<TH2>("tracks/eff/triton/h2pVsTPCmomentumTr", "#it{p}_{TPC} vs #it{p}; #it{p}_{TPC}; #it{p}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
        histos.add<TH2>("tracks/eff/triton/h2pVsTPCmomentumantiTr", "#it{p}_{TPC} vs #it{p}; #it{p}_{TPC}; #it{p}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
        if (doTOFplots) {
          histos.add<TH2>("tracks/eff/triton/h2pVsTOFExpMomentumTr", "#it{p}_{TOF} vs #it{p}; #it{p}_{TOF}; #it{p}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
          histos.add<TH2>("tracks/eff/triton/h2pVsTOFExpMomentumantiTr", "#it{p}_{TOF} vs #it{p}; #it{p}_{TOF}; #it{p}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
          histos.add<TH2>("tracks/eff/triton/h2TPCmomentumVsTOFExpMomentumTr", "#it{p}_{TOF} vs #it{p}_{TPC}; #it{p}_{TOF}; #it{p}_{TPC}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
          histos.add<TH2>("tracks/eff/triton/h2TPCmomentumVsTOFExpMomentumantiTr", "#it{p}_{TOF} vs #it{p}_{TPC}; #it{p}_{TOF}; #it{p}_{TPC}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
        }
      }
      if (enableHe) {
        histos.add<TH2>("tracks/eff/helium/h2pVsTPCmomentumHe", "#it{p}_{TPC} vs #it{p}; #it{p}_{TPC}; #it{p}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
        histos.add<TH2>("tracks/eff/helium/h2pVsTPCmomentumantiHe", "#it{p}_{TPC} vs #it{p}; #it{p}_{TPC}; #it{p}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
        if (doTOFplots) {
          histos.add<TH2>("tracks/eff/helium/h2pVsTOFExpMomentumHe", "#it{p}_{TOF} vs #it{p}; #it{p}_{TOF}; #it{p}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
          histos.add<TH2>("tracks/eff/helium/h2pVsTOFExpMomentumantiHe", "#it{p}_{TOF} vs #it{p}; #it{p}_{TOF}; #it{p}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
          histos.add<TH2>("tracks/eff/helium/h2TPCmomentumVsTOFExpMomentumHe", "#it{p}_{TOF} vs #it{p}_{TPC}; #it{p}_{TOF}; #it{p}_{TPC}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
          histos.add<TH2>("tracks/eff/helium/h2TPCmomentumVsTOFExpMomentumantiHe", "#it{p}_{TOF} vs #it{p}_{TPC}; #it{p}_{TOF}; #it{p}_{TPC}", HistType::kTH2F, {{800, 0.f, 8.f}, {800, 0.f, 8.f}});
        }
      }
    }

    if (enableDebug) {
      histos.add<TH1>("event/h1CentV0M", "V0M; Multiplicity; counts", HistType::kTH1F, {{27000, 0, 27000}});
      // trackQA
      histos.add<TH1>("tracks/h1Eta", "pseudoRapidity; #eta; counts", HistType::kTH1F, {{200, -1.0, 1.0}});
      histos.add<TH1>("tracks/h1VarPhi", "#phi; #phi; counts", HistType::kTH1F, {{63, 0.0, 6.3}});
      histos.add<TH2>("tracks/h2EtaVsPhi", "#eta vs #phi; #eta; #phi", HistType::kTH2F, {{200, -1.0, 1.0}, {63, 0.0, 6.3}});
    }

    if (enablePtSpectra) {
      if (enablePr) {
        histos.add<TH1>("tracks/eff/proton/hPtPr", "Track #it{p}_{T} (p); #it{p}_{T} (GeV/#it{c}); counts", HistType::kTH1F, {{400, 0., 8.}});
        histos.add<TH1>("tracks/eff/proton/hPtantiPr", "Track #it{p}_{T} (#bar{p}); #it{p}_{T} (GeV/#it{c}); counts", HistType::kTH1F, {{400, 0., 8.}});
        histos.add<TH1>("tracks/eff/proton/hPtPrTOF", "Track #it{p}_{T} (p); #it{p}_{T} (GeV/#it{c}); counts", HistType::kTH1F, {{400, 0., 8.}});
        histos.add<TH1>("tracks/eff/proton/hPtantiPrTOF", "Track #it{p}_{T} (#bar{p}); #it{p}_{T} (GeV/#it{c}); counts", HistType::kTH1F, {{400, 0., 8.}});
      }
      if (enableDe) {
        histos.add<TH1>("tracks/eff/deuteron/hPtDe", "Track #it{p}_{T} (d); #it{p}_{T} (GeV/#it{c}); counts", HistType::kTH1F, {{400, 0., 8.}});
        histos.add<TH1>("tracks/eff/deuteron/hPtantiDe", "Track #it{p}_{T} (#bar{d}); #it{p}_{T} (GeV/#it{c}); counts", HistType::kTH1F, {{400, 0., 8.}});
        histos.add<TH1>("tracks/eff/deuteron/hPtDeTOF", "Track #it{p}_{T} (d); #it{p}_{T} (GeV/#it{c}); counts", HistType::kTH1F, {{400, 0., 8.}});
        histos.add<TH1>("tracks/eff/deuteron/hPtantiDeTOF", "Track #it{p}_{T} (#bar{d}); #it{p}_{T} (GeV/#it{c}); counts", HistType::kTH1F, {{400, 0., 8.}});
      }
      if (enableTr) {
        histos.add<TH1>("tracks/eff/triton/hPtTr", "Track #it{p}_{T} (t); #it{p}_{T} (GeV/#it{c}); counts", HistType::kTH1F, {{400, 0., 8.}});
        histos.add<TH1>("tracks/eff/triton/hPtantiTr", "Track #it{p}_{T} (#bar{t}); #it{p}_{T} (GeV/#it{c}); counts", HistType::kTH1F, {{400, 0., 8.}});
        histos.add<TH1>("tracks/eff/triton/hPtTrTOF", "Track #it{p}_{T} (t); #it{p}_{T} (GeV/#it{c}); counts", HistType::kTH1F, {{400, 0., 8.}});
        histos.add<TH1>("tracks/eff/triton/hPtantiTrTOF", "Track #it{p}_{T} (#bar{t}); #it{p}_{T} (GeV/#it{c}); counts", HistType::kTH1F, {{400, 0., 8.}});
      }
      if (enableHe) {
        histos.add<TH1>("tracks/eff/helium/hPtHe", "Track #it{p}_{T} (He); #it{p}_{T} (GeV/#it{c}); counts", HistType::kTH1F, {{400, 0., 8.}});
        histos.add<TH1>("tracks/eff/helium/hPtantiHe", "Track #it{p}_{T} (#bar{He}); #it{p}_{T} (GeV/#it{c}); counts", HistType::kTH1F, {{400, 0., 8.}});
        histos.add<TH1>("tracks/eff/helium/hPtHeTOF", "Track #it{p}_{T} (He); #it{p}_{T} (GeV/#it{c}); counts", HistType::kTH1F, {{400, 0., 8.}});
        histos.add<TH1>("tracks/eff/helium/hPtantiHeTOF", "Track #it{p}_{T} (#bar{He}); #it{p}_{T} (GeV/#it{c}); counts", HistType::kTH1F, {{400, 0., 8.}});
      }
    }
    // tracks
    // DCAxy,z
    if (makeDCABeforeCutPlots) {
      histos.add<TH1>("tracks/dca/before/hDCAxy", "DCAxy", HistType::kTH1F, {dcaxyAxis});
      histos.add<TH1>("tracks/dca/before/hDCAz", "DCAz", HistType::kTH1F, {dcazAxis});
      histos.add<TH2>("tracks/dca/before/hDCAxyVsDCAz", "DCAxy vs DCAz", HistType::kTH2F, {{dcaxyAxis}, {dcazAxis}});
      histos.add<TH2>("tracks/dca/before/hDCAxyVsPt", "DCAxy vs Pt", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
      histos.add<TH2>("tracks/dca/before/hDCAzVsPt", "DCAz vs Pt", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

      if (enablePr) {
        histos.add<TH2>("tracks/proton/dca/before/hDCAxyVsPtProton", "DCAxy vs Pt (p)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/proton/dca/before/hDCAxyVsPtantiProton", "DCAxy vs Pt (#bar{p})", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/proton/dca/before/hDCAzVsPtProton", "DCAz vs Pt (p)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/proton/dca/before/hDCAzVsPtantiProton", "DCAz vs Pt (#bar{p})", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
      }
      if (enableDe) {
        histos.add<TH2>("tracks/deuteron/dca/before/hDCAxyVsPtDeuteron", "DCAxy vs Pt (d)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/deuteron/dca/before/hDCAxyVsPtantiDeuteron", "DCAxy vs Pt (#bar{d})", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/deuteron/dca/before/hDCAzVsPtDeuteron", "DCAz vs Pt (d)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/deuteron/dca/before/hDCAzVsPtantiDeuteron", "DCAz vs Pt (#bar{d})", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
      }
      if (enableTr) {
        histos.add<TH2>("tracks/triton/dca/before/hDCAxyVsPtTriton", "DCAxy vs Pt (t)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/triton/dca/before/hDCAxyVsPtantiTriton", "DCAxy vs Pt (#bar{t})", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/triton/dca/before/hDCAzVsPtTriton", "DCAz vs Pt (t)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/triton/dca/before/hDCAzVsPtantiTriton", "DCAz vs Pt (#bar{t})", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
      }
      if (enableHe) {
        histos.add<TH2>("tracks/helium/dca/before/hDCAxyVsPtHelium", "DCAxy vs Pt (He)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/helium/dca/before/hDCAxyVsPtantiHelium", "DCAxy vs Pt (#bar{He})", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/helium/dca/before/hDCAzVsPtHelium", "DCAz vs Pt (He)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/helium/dca/before/hDCAzVsPtantiHelium", "DCAz vs Pt (#bar{He})", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
      }
      if (enableAl) {
        histos.add<TH2>("tracks/alpha/dca/before/hDCAxyVsPtAlpha", "DCAxy vs Pt (#alpha)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/alpha/dca/before/hDCAxyVsPtantiAlpha", "DCAxy vs Pt (#bar{#alpha})", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/alpha/dca/before/hDCAzVsPtAlpha", "DCAz vs Pt (#alpha)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/alpha/dca/before/hDCAzVsPtantiAlpha", "DCAz vs Pt (#bar{#alpha})", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
      }
    }

    if (makeDCAAfterCutPlots) {
      histos.add<TH1>("tracks/dca/after/hDCAxy", "DCAxy; DCAxy; counts", HistType::kTH1F, {{dcaxyAxis}});
      histos.add<TH1>("tracks/dca/after/hDCAz", "DCAz; DCAz; counts", HistType::kTH1F, {{dcazAxis}});
      histos.add<TH2>("tracks/dca/after/hDCAxyVsDCAz", "DCAxy vs DCAz; DCAxy (cm); DCAz (cm)", HistType::kTH2F, {{dcaxyAxis}, {dcazAxis}});
      histos.add<TH2>("tracks/dca/after/hDCAxyVsPt", "DCAxy vs Pt", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
      histos.add<TH2>("tracks/dca/after/hDCAzVsPt", "DCAz vs Pt", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

      if (enablePr) {
        histos.add<TH2>("tracks/proton/dca/after/hDCAxyVsPtProton", "DCAxy vs Pt (p)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/proton/dca/after/hDCAxyVsPtantiProton", "DCAxy vs Pt (#bar{p})", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/proton/dca/after/hDCAzVsPtProton", "DCAz vs Pt (p)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/proton/dca/after/hDCAzVsPtantiProton", "DCAz vs Pt (#bar{p})", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
      }
      if (enableDe) {
        histos.add<TH2>("tracks/deuteron/dca/after/hDCAxyVsPtDeuteron", "DCAxy vs Pt (d)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/deuteron/dca/after/hDCAxyVsPtantiDeuteron", "DCAxy vs Pt (#bar{d})", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/deuteron/dca/after/hDCAzVsPtDeuteron", "DCAz vs Pt (d)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/deuteron/dca/after/hDCAzVsPtantiDeuteron", "DCAz vs Pt (#bar{d})", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
      }
      if (enableTr) {
        histos.add<TH2>("tracks/triton/dca/after/hDCAxyVsPtTriton", "DCAxy vs Pt (t)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/triton/dca/after/hDCAxyVsPtantiTriton", "DCAxy vs Pt (#bar{t})", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/triton/dca/after/hDCAzVsPtTriton", "DCAz vs Pt (t)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/triton/dca/after/hDCAzVsPtantiTriton", "DCAz vs Pt (#bar{t})", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
      }
      if (enableHe) {
        histos.add<TH2>("tracks/helium/dca/after/hDCAxyVsPtHelium", "DCAxy vs Pt (He)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/helium/dca/after/hDCAxyVsPtantiHelium", "DCAxy vs Pt (#bar{He})", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/helium/dca/after/hDCAzVsPtHelium", "DCAz vs Pt (He)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/helium/dca/after/hDCAzVsPtantiHelium", "DCAz vs Pt (#bar{He})", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
      }
      if (enableAl) {
        histos.add<TH2>("tracks/alpha/dca/after/hDCAxyVsPtAlpha", "DCAxy vs Pt (#alpha)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/alpha/dca/after/hDCAxyVsPtantiAlpha", "DCAxy vs Pt (#bar{#alpha})", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/alpha/dca/after/hDCAzVsPtAlpha", "DCAz vs Pt (#alpha)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/alpha/dca/after/hDCAzVsPtantiAlpha", "DCAz vs Pt (#bar{#alpha})", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
      }
    }

    // Spectra (After cuts)
    if (enablePr) {
      histos.add<TH1>("tracks/proton/h1ProtonSpectra", "#it{p}_{T} (p)", HistType::kTH1F, {ptAxis});
      histos.add<TH1>("tracks/proton/h1antiProtonSpectra", "#it{p}_{T} (#bar{p})", HistType::kTH1F, {ptAxis});
    }
    if (enableDe) {
      histos.add<TH1>("tracks/deuteron/h1DeuteronSpectra", "#it{p}_{T} (d)", HistType::kTH1F, {ptAxis});
      histos.add<TH1>("tracks/deuteron/h1antiDeuteronSpectra", "#it{p}_{T} (#bar{d})", HistType::kTH1F, {ptAxis});
    }
    if (enableTr) {
      histos.add<TH1>("tracks/triton/h1TritonSpectra", "#it{p}_{T} (t)", HistType::kTH1F, {ptAxis});
      histos.add<TH1>("tracks/triton/h1antiTritonSpectra", "#it{p}_{T} (#bar{t})", HistType::kTH1F, {ptAxis});
    }
    if (enableHe) {
      histos.add<TH1>("tracks/helium/h1HeliumSpectra", "#it{p}_{T} (He)", HistType::kTH1F, {ptAxis});
      histos.add<TH1>("tracks/helium/h1antiHeliumSpectra", "#it{p}_{T} (#bar{He})", HistType::kTH1F, {ptAxis});
    }
    if (enableAl) {
      histos.add<TH1>("tracks/alpha/h1AlphaSpectra", "#it{p}_{T} (#alpha)", HistType::kTH1F, {ptAxis});
      histos.add<TH1>("tracks/alpha/h1antiAlphaSpectra", "#it{p}_{T} (#bar{#alpha})", HistType::kTH1F, {ptAxis});
    }
    if (doprocessMCReco || doprocessMCRecoFiltered) {
      // 1D pT
      if (enablePr) {
        histos.add<TH1>("tracks/proton/h1ProtonSpectraTrue", "#it{p}_{T} (p)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/proton/h1ProtonSpectraTrueWPID", "#it{p}_{T} (p)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/proton/h1ProtonSpectraTruePrim", "#it{p}_{T} (p)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/proton/h1ProtonSpectraTrueSec", "#it{p}_{T} (p)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/proton/h1ProtonSpectraTrueTransport", "#it{p}_{T} (p)", HistType::kTH1F, {ptAxis});

        histos.add<TH1>("tracks/proton/h1antiProtonSpectraTrue", "#it{p}_{T} (#bar{p})", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/proton/h1antiProtonSpectraTrueWPID", "#it{p}_{T} (#bar{p})", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/proton/h1antiProtonSpectraTruePrim", "#it{p}_{T} (#bar{p})", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/proton/h1antiProtonSpectraTrueSec", "#it{p}_{T} (#bar{p})", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/proton/h1antiProtonSpectraTrueTransport", "#it{p}_{T} (#bar{p})", HistType::kTH1F, {ptAxis});
      }
      if (enableDe) {
        histos.add<TH1>("tracks/deuteron/h1DeuteronSpectraTrue", "#it{p}_{T} (d)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/deuteron/h1DeuteronSpectraTrueWPID", "#it{p}_{T} (d)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/deuteron/h1DeuteronSpectraTruePrim", "#it{p}_{T} (d)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/deuteron/h1DeuteronSpectraTrueSec", "#it{p}_{T} (d)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/deuteron/h1DeuteronSpectraTrueTransport", "#it{p}_{T} (d)", HistType::kTH1F, {ptAxis});

        histos.add<TH1>("tracks/deuteron/h1antiDeuteronSpectraTrue", "#it{p}_{T} (#bar{d})", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/deuteron/h1antiDeuteronSpectraTrueWPID", "#it{p}_{T} (#bar{d})", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/deuteron/h1antiDeuteronSpectraTruePrim", "#it{p}_{T} (#bar{d})", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/deuteron/h1antiDeuteronSpectraTrueSec", "#it{p}_{T} (#bar{d})", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/deuteron/h1antiDeuteronSpectraTrueTransport", "#it{p}_{T} (#bar{d})", HistType::kTH1F, {ptAxis});
      }
      if (enableTr) {
        histos.add<TH1>("tracks/triton/h1TritonSpectraTrue", "#it{p}_{T} (t)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/triton/h1TritonSpectraTruePrim", "#it{p}_{T} (t)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/triton/h1TritonSpectraTrueSec", "#it{p}_{T} (t)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/triton/h1TritonSpectraTrueTransport", "#it{p}_{T} (t)", HistType::kTH1F, {ptAxis});

        histos.add<TH1>("tracks/triton/h1antiTritonSpectraTrue", "#it{p}_{T} (#bar{t})", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/triton/h1antiTritonSpectraTruePrim", "#it{p}_{T} (#bar{t})", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/triton/h1antiTritonSpectraTrueSec", "#it{p}_{T} (#bar{t})", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/triton/h1antiTritonSpectraTrueTransport", "#it{p}_{T} (#bar{t})", HistType::kTH1F, {ptAxis});
      }
      if (enableHe) {
        histos.add<TH1>("tracks/helium/h1HeliumSpectraTrue", "#it{p}_{T} (He)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/helium/h1HeliumSpectraTrueWPID", "#it{p}_{T} (He)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/helium/h1HeliumSpectraTruePrim", "#it{p}_{T} (He)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/helium/h1HeliumSpectraTrueSec", "#it{p}_{T} (He)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/helium/h1HeliumSpectraTrueTransport", "#it{p}_{T} (He)", HistType::kTH1F, {ptAxis});

        histos.add<TH1>("tracks/helium/h1antiHeliumSpectraTrue", "#it{p}_{T} (He)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/helium/h1antiHeliumSpectraTrueWPID", "#it{p}_{T} (He)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/helium/h1antiHeliumSpectraTruePrim", "#it{p}_{T} (He)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/helium/h1antiHeliumSpectraTrueSec", "#it{p}_{T} (He)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/helium/h1antiHeliumSpectraTrueTransport", "#it{p}_{T} (He)", HistType::kTH1F, {ptAxis});
      }
      if (enableAl) {
        histos.add<TH1>("tracks/alpha/h1AlphaSpectraTrue", "#it{p}_{T} (#alpha)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/alpha/h1AlphaSpectraTruePrim", "#it{p}_{T} (#alpha)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/alpha/h1AlphaSpectraTrueSec", "#it{p}_{T} (#alpha)", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/alpha/h1AlphaSpectraTrueTransport", "#it{p}_{T} (#alpha)", HistType::kTH1F, {ptAxis});

        histos.add<TH1>("tracks/alpha/h1antiAlphaSpectraTrue", "#it{p}_{T} (#bar{#alpha})", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/alpha/h1antiAlphaSpectraTruePrim", "#it{p}_{T} (#bar{#alpha})", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/alpha/h1antiAlphaSpectraTrueSec", "#it{p}_{T} (#bar{#alpha})", HistType::kTH1F, {ptAxis});
        histos.add<TH1>("tracks/alpha/h1antiAlphaSpectraTrueTransport", "#it{p}_{T} (#bar{#alpha})", HistType::kTH1F, {ptAxis});
      }
      // 2D-DCAxy
      if (enablePr) {
        histos.add<TH2>("tracks/proton/dca/before/hDCAxyVsPtProtonTrue", "DCAxy vs Pt (p); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/proton/dca/before/hDCAxyVsPtProtonTruePrim", "DCAxy vs Pt (p); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/proton/dca/before/hDCAxyVsPtProtonTrueSec", "DCAxy vs Pt (p); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/proton/dca/before/hDCAxyVsPtProtonTrueTransport", "DCAxy vs Pt (p); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/proton/dca/before/hDCAxyVsPtantiProtonTrue", "DCAxy vs Pt (#bar{p}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/proton/dca/before/hDCAxyVsPtantiProtonTruePrim", "DCAxy vs Pt (#bar{p}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/proton/dca/before/hDCAxyVsPtantiProtonTrueSec", "DCAxy vs Pt (#bar{p}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/proton/dca/before/hDCAxyVsPtantiProtonTrueTransport", "DCAxy vs Pt (#bar{p}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/proton/dca/after/hDCAxyVsPtProtonTrue", "DCAxy vs Pt (p); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/proton/dca/after/hDCAxyVsPtProtonTruePrim", "DCAxy vs Pt (p); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/proton/dca/after/hDCAxyVsPtProtonTrueSec", "DCAxy vs Pt (p); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/proton/dca/after/hDCAxyVsPtProtonTrueTransport", "DCAxy vs Pt (p); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/proton/dca/after/hDCAxyVsPtantiProtonTrue", "DCAxy vs Pt (#bar{p}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/proton/dca/after/hDCAxyVsPtantiProtonTruePrim", "DCAxy vs Pt (#bar{p}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/proton/dca/after/hDCAxyVsPtantiProtonTrueSec", "DCAxy vs Pt (#bar{p}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/proton/dca/after/hDCAxyVsPtantiProtonTrueTransport", "DCAxy vs Pt (#bar{p}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
      }
      if (enableDe) {
        histos.add<TH2>("tracks/deuteron/dca/before/hDCAxyVsPtDeuteronTrue", "DCAxy vs Pt (d); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/deuteron/dca/before/hDCAxyVsPtDeuteronTruePrim", "DCAxy vs Pt (d); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/deuteron/dca/before/hDCAxyVsPtDeuteronTrueSec", "DCAxy vs Pt (d); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/deuteron/dca/before/hDCAxyVsPtDeuteronTrueTransport", "DCAxy vs Pt (d); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/deuteron/dca/before/hDCAxyVsPtantiDeuteronTrue", "DCAxy vs Pt (#bar{d}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/deuteron/dca/before/hDCAxyVsPtantiDeuteronTruePrim", "DCAxy vs Pt (#bar{d}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/deuteron/dca/before/hDCAxyVsPtantiDeuteronTrueSec", "DCAxy vs Pt (#bar{d}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/deuteron/dca/before/hDCAxyVsPtantiDeuteronTrueTransport", "DCAxy vs Pt (#bar{d}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/deuteron/dca/after/hDCAxyVsPtDeuteronTrue", "DCAxy vs Pt (d); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/deuteron/dca/after/hDCAxyVsPtDeuteronTruePrim", "DCAxy vs Pt (d); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/deuteron/dca/after/hDCAxyVsPtDeuteronTrueSec", "DCAxy vs Pt (d); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/deuteron/dca/after/hDCAxyVsPtDeuteronTrueTransport", "DCAxy vs Pt (d); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/deuteron/dca/after/hDCAxyVsPtantiDeuteronTrue", "DCAxy vs Pt (#bar{d}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/deuteron/dca/after/hDCAxyVsPtantiDeuteronTruePrim", "DCAxy vs Pt (#bar{d}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/deuteron/dca/after/hDCAxyVsPtantiDeuteronTrueSec", "DCAxy vs Pt (#bar{d}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/deuteron/dca/after/hDCAxyVsPtantiDeuteronTrueTransport", "DCAxy vs Pt (#bar{d}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
      }
      if (enableTr) {
        histos.add<TH2>("tracks/triton/dca/before/hDCAxyVsPtTritonTrue", "DCAxy vs Pt (t); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/triton/dca/before/hDCAxyVsPtTritonTruePrim", "DCAxy vs Pt (t); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/triton/dca/before/hDCAxyVsPtTritonTrueSec", "DCAxy vs Pt (t); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/triton/dca/before/hDCAxyVsPtTritonTrueTransport", "DCAxy vs Pt (t); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/triton/dca/before/hDCAxyVsPtantiTritonTrue", "DCAxy vs Pt (#bar{t}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/triton/dca/before/hDCAxyVsPtantiTritonTruePrim", "DCAxy vs Pt (#bar{t}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/triton/dca/before/hDCAxyVsPtantiTritonTrueSec", "DCAxy vs Pt (#bar{t}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/triton/dca/before/hDCAxyVsPtantiTritonTrueTransport", "DCAxy vs Pt (#bar{t}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/triton/dca/after/hDCAxyVsPtTritonTrue", "DCAxy vs Pt (t); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/triton/dca/after/hDCAxyVsPtTritonTruePrim", "DCAxy vs Pt (t); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/triton/dca/after/hDCAxyVsPtTritonTrueSec", "DCAxy vs Pt (t); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/triton/dca/after/hDCAxyVsPtTritonTrueTransport", "DCAxy vs Pt (t); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/triton/dca/after/hDCAxyVsPtantiTritonTrue", "DCAxy vs Pt (#bar{t}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/triton/dca/after/hDCAxyVsPtantiTritonTruePrim", "DCAxy vs Pt (#bar{t}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/triton/dca/after/hDCAxyVsPtantiTritonTrueSec", "DCAxy vs Pt (#bar{t}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/triton/dca/after/hDCAxyVsPtantiTritonTrueTransport", "DCAxy vs Pt (#bar{t}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
      }
      if (enableHe) {
        histos.add<TH2>("tracks/helium/dca/before/hDCAxyVsPtHeliumTrue", "DCAxy vs Pt (He); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/helium/dca/before/hDCAxyVsPtHeliumTruePrim", "DCAxy vs Pt (He); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/helium/dca/before/hDCAxyVsPtHeliumTrueSec", "DCAxy vs Pt (He); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/helium/dca/before/hDCAxyVsPtHeliumTrueTransport", "DCAxy vs Pt (He); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/helium/dca/before/hDCAxyVsPtantiHeliumTrue", "DCAxy vs Pt (#bar{He}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/helium/dca/before/hDCAxyVsPtantiHeliumTruePrim", "DCAxy vs Pt (#bar{He}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/helium/dca/before/hDCAxyVsPtantiHeliumTrueSec", "DCAxy vs Pt (#bar{He}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/helium/dca/before/hDCAxyVsPtantiHeliumTrueTransport", "DCAxy vs Pt (#bar{He}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/helium/dca/after/hDCAxyVsPtHeliumTrue", "DCAxy vs Pt (He); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/helium/dca/after/hDCAxyVsPtHeliumTruePrim", "DCAxy vs Pt (He); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/helium/dca/after/hDCAxyVsPtHeliumTrueSec", "DCAxy vs Pt (He); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/helium/dca/after/hDCAxyVsPtHeliumTrueTransport", "DCAxy vs Pt (He); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/helium/dca/after/hDCAxyVsPtantiHeliumTrue", "DCAxy vs Pt (#bar{He}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/helium/dca/after/hDCAxyVsPtantiHeliumTruePrim", "DCAxy vs Pt (#bar{He}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/helium/dca/after/hDCAxyVsPtantiHeliumTrueSec", "DCAxy vs Pt (#bar{He}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/helium/dca/after/hDCAxyVsPtantiHeliumTrueTransport", "DCAxy vs Pt (#bar{He}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
      }
      if (enableAl) {
        histos.add<TH2>("tracks/alpha/dca/before/hDCAxyVsPtAlphaTrue", "DCAxy vs Pt (#alpha); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/alpha/dca/before/hDCAxyVsPtAlphaTruePrim", "DCAxy vs Pt (#alpha); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/alpha/dca/before/hDCAxyVsPtAlphaTrueSec", "DCAxy vs Pt (#alpha); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/alpha/dca/before/hDCAxyVsPtAlphaTrueTransport", "DCAxy vs Pt (#alpha); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/alpha/dca/before/hDCAxyVsPtantiAlphaTrue", "DCAxy vs Pt (#bar{#alpha}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/alpha/dca/before/hDCAxyVsPtantiAlphaTruePrim", "DCAxy vs Pt (#bar{#alpha}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/alpha/dca/before/hDCAxyVsPtantiAlphaTrueSec", "DCAxy vs Pt (#bar{#alpha}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/alpha/dca/before/hDCAxyVsPtantiAlphaTrueTransport", "DCAxy vs Pt (#bar{#alpha}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/alpha/dca/after/hDCAxyVsPtAlphaTrue", "DCAxy vs Pt (#alpha); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/alpha/dca/after/hDCAxyVsPtAlphaTruePrim", "DCAxy vs Pt (#alpha); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/alpha/dca/after/hDCAxyVsPtAlphaTrueSec", "DCAxy vs Pt (#alpha); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/alpha/dca/after/hDCAxyVsPtAlphaTrueTransport", "DCAxy vs Pt (#alpha); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/alpha/dca/after/hDCAxyVsPtantiAlphaTrue", "DCAxy vs Pt (#bar{#alpha}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});

        histos.add<TH2>("tracks/alpha/dca/after/hDCAxyVsPtantiAlphaTruePrim", "DCAxy vs Pt (#bar{#alpha}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/alpha/dca/after/hDCAxyVsPtantiAlphaTrueSec", "DCAxy vs Pt (#bar{#alpha}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
        histos.add<TH2>("tracks/alpha/dca/after/hDCAxyVsPtantiAlphaTrueTransport", "DCAxy vs Pt (#bar{#alpha}); #it{p}_{T} (GeV/#it{c}); DCAxy (cm)", HistType::kTH2F, {{ptAxis}, {dcaxyAxis}});
      }
      // 2D-DCAz
      if (enablePr) {
        histos.add<TH2>("tracks/proton/dca/before/hDCAzVsPtProtonTrue", "DCAz vs Pt (p); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/proton/dca/before/hDCAzVsPtProtonTruePrim", "DCAz vs Pt (p); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/proton/dca/before/hDCAzVsPtProtonTrueSec", "DCAz vs Pt (p); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/proton/dca/before/hDCAzVsPtProtonTrueTransport", "DCAz vs Pt (p); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/proton/dca/before/hDCAzVsPtantiProtonTrue", "DCAz vs Pt (#bar{p}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/proton/dca/before/hDCAzVsPtantiProtonTruePrim", "DCAz vs Pt (#bar{p}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/proton/dca/before/hDCAzVsPtantiProtonTrueSec", "DCAz vs Pt (#bar{p}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/proton/dca/before/hDCAzVsPtantiProtonTrueTransport", "DCAz vs Pt (#bar{p}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/proton/dca/after/hDCAzVsPtProtonTrue", "DCAz vs Pt (p); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/proton/dca/after/hDCAzVsPtProtonTruePrim", "DCAz vs Pt (p); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/proton/dca/after/hDCAzVsPtProtonTrueSec", "DCAz vs Pt (p); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/proton/dca/after/hDCAzVsPtProtonTrueTransport", "DCAz vs Pt (p); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/proton/dca/after/hDCAzVsPtantiProtonTrue", "DCAz vs Pt (#bar{p}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/proton/dca/after/hDCAzVsPtantiProtonTruePrim", "DCAz vs Pt (#bar{p}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/proton/dca/after/hDCAzVsPtantiProtonTrueSec", "DCAz vs Pt (#bar{p}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/proton/dca/after/hDCAzVsPtantiProtonTrueTransport", "DCAz vs Pt (#bar{p}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
      }
      if (enableDe) {
        histos.add<TH2>("tracks/deuteron/dca/before/hDCAzVsPtDeuteronTrue", "DCAz vs Pt (d); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/deuteron/dca/before/hDCAzVsPtDeuteronTruePrim", "DCAz vs Pt (d); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/deuteron/dca/before/hDCAzVsPtDeuteronTrueSec", "DCAz vs Pt (d); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/deuteron/dca/before/hDCAzVsPtDeuteronTrueTransport", "DCAz vs Pt (d); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/deuteron/dca/before/hDCAzVsPtantiDeuteronTrue", "DCAz vs Pt (#bar{d}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/deuteron/dca/before/hDCAzVsPtantiDeuteronTruePrim", "DCAz vs Pt (#bar{d}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/deuteron/dca/before/hDCAzVsPtantiDeuteronTrueSec", "DCAz vs Pt (#bar{d}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/deuteron/dca/before/hDCAzVsPtantiDeuteronTrueTransport", "DCAz vs Pt (#bar{d}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/deuteron/dca/after/hDCAzVsPtDeuteronTrue", "DCAz vs Pt (d); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/deuteron/dca/after/hDCAzVsPtDeuteronTruePrim", "DCAz vs Pt (d); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/deuteron/dca/after/hDCAzVsPtDeuteronTrueSec", "DCAz vs Pt (d); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/deuteron/dca/after/hDCAzVsPtDeuteronTrueTransport", "DCAz vs Pt (d); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/deuteron/dca/after/hDCAzVsPtantiDeuteronTrue", "DCAz vs Pt (#bar{d}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/deuteron/dca/after/hDCAzVsPtantiDeuteronTruePrim", "DCAz vs Pt (#bar{d}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/deuteron/dca/after/hDCAzVsPtantiDeuteronTrueSec", "DCAz vs Pt (#bar{d}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/deuteron/dca/after/hDCAzVsPtantiDeuteronTrueTransport", "DCAz vs Pt (#bar{d}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
      }
      if (enableTr) {
        histos.add<TH2>("tracks/triton/dca/before/hDCAzVsPtTritonTrue", "DCAz vs Pt (t); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/triton/dca/before/hDCAzVsPtTritonTruePrim", "DCAz vs Pt (t); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/triton/dca/before/hDCAzVsPtTritonTrueSec", "DCAz vs Pt (t); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/triton/dca/before/hDCAzVsPtTritonTrueTransport", "DCAz vs Pt (t); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/triton/dca/before/hDCAzVsPtantiTritonTrue", "DCAz vs Pt (#bar{t}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/triton/dca/before/hDCAzVsPtantiTritonTruePrim", "DCAz vs Pt (#bar{t}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/triton/dca/before/hDCAzVsPtantiTritonTrueSec", "DCAz vs Pt (#bar{t}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/triton/dca/before/hDCAzVsPtantiTritonTrueTransport", "DCAz vs Pt (#bar{t}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/triton/dca/before/hDCAzVsPtTritonTrue", "DCAz vs Pt (t); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/triton/dca/after/hDCAzVsPtTritonTruePrim", "DCAz vs Pt (t); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/triton/dca/after/hDCAzVsPtTritonTrueSec", "DCAz vs Pt (t); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/triton/dca/after/hDCAzVsPtTritonTrueTransport", "DCAz vs Pt (t); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/triton/dca/after/hDCAzVsPtantiTritonTrue", "DCAz vs Pt (#bar{t}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/triton/dca/after/hDCAzVsPtantiTritonTruePrim", "DCAz vs Pt (#bar{t}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/triton/dca/after/hDCAzVsPtantiTritonTrueSec", "DCAz vs Pt (#bar{t}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/triton/dca/after/hDCAzVsPtantiTritonTrueTransport", "DCAz vs Pt (#bar{t}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
      }
      if (enableHe) {
        histos.add<TH2>("tracks/helium/dca/before/hDCAzVsPtHeliumTrue", "DCAz vs Pt (He); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/helium/dca/before/hDCAzVsPtHeliumTruePrim", "DCAz vs Pt (He); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/helium/dca/before/hDCAzVsPtHeliumTrueSec", "DCAz vs Pt (He); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/helium/dca/before/hDCAzVsPtHeliumTrueTransport", "DCAz vs Pt (He); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/helium/dca/before/hDCAzVsPtantiHeliumTrue", "DCAz vs Pt (#bar{He}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/helium/dca/before/hDCAzVsPtantiHeliumTruePrim", "DCAz vs Pt (#bar{He}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/helium/dca/before/hDCAzVsPtantiHeliumTrueSec", "DCAz vs Pt (#bar{He}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/helium/dca/before/hDCAzVsPtantiHeliumTrueTransport", "DCAz vs Pt (#bar{He}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/helium/dca/before/hDCAzVsPtHeliumTrue", "DCAz vs Pt (He); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/helium/dca/after/hDCAzVsPtHeliumTruePrim", "DCAz vs Pt (He); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/helium/dca/after/hDCAzVsPtHeliumTrueSec", "DCAz vs Pt (He); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/helium/dca/after/hDCAzVsPtHeliumTrueTransport", "DCAz vs Pt (He); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/helium/dca/after/hDCAzVsPtantiHeliumTrue", "DCAz vs Pt (#bar{He}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/helium/dca/after/hDCAzVsPtantiHeliumTruePrim", "DCAz vs Pt (#bar{He}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/helium/dca/after/hDCAzVsPtantiHeliumTrueSec", "DCAz vs Pt (#bar{He}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/helium/dca/after/hDCAzVsPtantiHeliumTrueTransport", "DCAz vs Pt (#bar{He}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
      }
      if (enableAl) {
        histos.add<TH2>("tracks/alpha/dca/before/hDCAzVsPtAlphaTrue", "DCAz vs Pt (#alpha); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/alpha/dca/before/hDCAzVsPtAlphaTruePrim", "DCAz vs Pt (#alpha); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/alpha/dca/before/hDCAzVsPtAlphaTrueSec", "DCAz vs Pt (#alpha); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/alpha/dca/before/hDCAzVsPtAlphaTrueTransport", "DCAz vs Pt (#alpha); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/alpha/dca/before/hDCAzVsPtantiAlphaTrue", "DCAz vs Pt (#bar{#alpha}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/alpha/dca/before/hDCAzVsPtantiAlphaTruePrim", "DCAz vs Pt (#bar{#alpha}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/alpha/dca/before/hDCAzVsPtantiAlphaTrueSec", "DCAz vs Pt (#bar{#alpha}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/alpha/dca/before/hDCAzVsPtantiAlphaTrueTransport", "DCAz vs Pt (#bar{#alpha}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/alpha/dca/before/hDCAzVsPtAlphaTrue", "DCAz vs Pt (#alpha); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/alpha/dca/after/hDCAzVsPtAlphaTruePrim", "DCAz vs Pt (#alpha); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/alpha/dca/after/hDCAzVsPtAlphaTrueSec", "DCAz vs Pt (#alpha); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/alpha/dca/after/hDCAzVsPtAlphaTrueTransport", "DCAz vs Pt (#alpha); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/alpha/dca/after/hDCAzVsPtantiAlphaTrue", "DCAz vs Pt (#bar{#alpha}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});

        histos.add<TH2>("tracks/alpha/dca/after/hDCAzVsPtantiAlphaTruePrim", "DCAz vs Pt (#bar{#alpha}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/alpha/dca/after/hDCAzVsPtantiAlphaTrueSec", "DCAz vs Pt (#bar{#alpha}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
        histos.add<TH2>("tracks/alpha/dca/after/hDCAzVsPtantiAlphaTrueTransport", "DCAz vs Pt (#bar{#alpha}); #it{p}_{T} (GeV/#it{c}); DCAz (cm)", HistType::kTH2F, {{ptAxis}, {dcazAxis}});
      }
    }

    //  Bethe-Bloch TPC distribution and Beta vs pT TOF distribution
    histos.add<TH2>("tracks/h2TPCsignVsTPCmomentum", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{500, -5.f, 5.f}, {dedxAxis}});
    if (enableDebug)
      histos.add<TH2>("debug/h2TPCsignVsTPCmomentumFull", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{500, -5.f, 5.f}, {dedxAxis}});

    if (enablePIDplot) {
      if (enablePr) {
        histos.add<TH2>("tracks/proton/h2TPCsignVsTPCmomentumProton", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
        histos.add<TH2>("tracks/proton/h2TPCsignVsTPCmomentumantiProton", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
      }
      if (enableDe) {
        histos.add<TH2>("tracks/deuteron/h2TPCsignVsTPCmomentumDeuteron", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
        histos.add<TH2>("tracks/deuteron/h2TPCsignVsTPCmomentumantiDeuteron", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
      }
      if (enableTr) {
        histos.add<TH2>("tracks/triton/h2TPCsignVsTPCmomentumTriton", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
        histos.add<TH2>("tracks/triton/h2TPCsignVsTPCmomentumantiTriton", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
      }
      if (enableHe) {
        histos.add<TH2>("tracks/helium/h2TPCsignVsTPCmomentumHelium", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
        histos.add<TH2>("tracks/helium/h2TPCsignVsTPCmomentumantiHelium", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
      }
      if (enableAl) {
        histos.add<TH2>("tracks/alpha/h2TPCsignVsTPCmomentumAlpha", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
        histos.add<TH2>("tracks/alpha/h2TPCsignVsTPCmomentumantiAlpha", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
      }
    }

    if (doTOFplots) {
      histos.add<TH2>("tracks/h2TPCsignVsBetaGamma", "TPC <-dE/dX> vs #beta#gamma/Z; Signed #beta#gamma; TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{600, -6.f, 6.f}, {dedxAxis}});
      histos.add<TH2>("tracks/h2TOFbetaVsP", "TOF #beta vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TOF #beta", HistType::kTH2F, {{500, -5.f, 5.f}, {betaAxis}});
      histos.add<TH2>("tracks/h2TOFbetaVsP_debug", "TOF #beta vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TOF #beta", HistType::kTH2F, {{500, -5.f, 5.f}, {betaAxis}});
      histos.add<TH2>("tracks/h2withTPCProtonCutTOFbetaVsP", "TOF #beta (with N#sigma_{TPC}(p)) cut) vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TOF #beta (with N#sigma_{TPC}(p)) cut)", HistType::kTH2F, {{500, -5.f, 5.f}, {betaAxis}});
      if (enableBetaCut)
        histos.add<TH2>("tracks/h2TOFbetaVsP_BetaCut", "TOF #beta vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TOF #beta", HistType::kTH2F, {{500, -5.f, 5.f}, {betaAxis}});
    }

    if (enableExpSignalTPC) {
      //  TPCExpSignal histograms
      if (enablePr) {
        histos.add<TH2>("tracks/proton/h2ProtonTPCExpSignalDiffVsPt", "TPC <-dE/dX> - Exp <-dE/dX> (p) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TPC <-dE/dX> ExpDiff (p)", HistType::kTH2F, {{ptAxis}, {16000, -800, 800.}});
        histos.add<TH2>("tracks/proton/h2antiProtonTPCExpSignalDiffVsPt", "TPC <-dE/dX> - Exp <-dE/dX> (#bar{p}) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TPC <-dE/dX> ExpDiff (#bar{p})", HistType::kTH2F, {{ptAxis}, {16000, -800, 800.}});
      }
      if (enableDe) {
        histos.add<TH2>("tracks/deuteron/h2DeuteronTPCExpSignalDiffVsPt", "TPC <-dE/dX> - Exp <-dE/dX> (d) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TPC <-dE/dX> ExpDiff (d)", HistType::kTH2F, {{ptAxis}, {16000, -800, 800.}});
        histos.add<TH2>("tracks/deuteron/h2antiDeuteronTPCExpSignalDiffVsPt", "TPC <-dE/dX> - Exp <-dE/dX> (#bar{d}) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TPC <-dE/dX> ExpDiff (#bar{d})", HistType::kTH2F, {{ptAxis}, {16000, -800, 800.}});
      }
      if (enableHe) {
        histos.add<TH2>("tracks/helium/h2HeliumTPCExpSignalDiffVsPt", "TPC <-dE/dX> - Exp <-dE/dX> (He) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TPC <-dE/dX> ExpDiff (He)", HistType::kTH2F, {{ptAxis}, {16000, -800, 800.}});
        histos.add<TH2>("tracks/helium/h2antiHeliumTPCExpSignalDiffVsPt", "TPC <-dE/dX> - Exp <-dE/dX> (#bar{He}) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TPC <-dE/dX> ExpDiff (#bar{He})", HistType::kTH2F, {{ptAxis}, {16000, -800, 800.}});
      }
    }

    //  NSigmasTPC histograms
    if (enableDebug) {
      histos.add<TH2>("tracks/pion/h2PionVspTNSigmaTPC", "NSigmaTPC(pi) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
      histos.add<TH2>("tracks/kaon/h2KaonVspTNSigmaTPC", "NSigmaTPC(Ka) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
    }

    if (enablePr) {
      histos.add<TH2>("tracks/proton/h2ProtonVspTNSigmaTPC", "NSigmaTPC(p) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
      histos.add<TH2>("tracks/proton/h2antiProtonVspTNSigmaTPC", "NSigmaTPC(#bar{p}) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
    }
    if (enableDe) {
      histos.add<TH2>("tracks/deuteron/h2DeuteronVspTNSigmaTPC", "NSigmaTPC(d) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
      histos.add<TH2>("tracks/deuteron/h2antiDeuteronVspTNSigmaTPC", "NSigmaTPC(#bar{d}) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
    }
    if (enableTr) {
      histos.add<TH2>("tracks/triton/h2TritonVspTNSigmaTPC", "NSigmaTPC(t) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
      histos.add<TH2>("tracks/triton/h2antiTritonVspTNSigmaTPC", "NSigmaTPC(#bar{t}) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
    }
    if (enableHe) {
      histos.add<TH2>("tracks/helium/h2HeliumVspTNSigmaTPC", "NSigmaTPC(He) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
      histos.add<TH2>("tracks/helium/h2antiHeliumVspTNSigmaTPC", "NSigmaTPC(#bar{He}) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
    }
    if (enableAl) {
      histos.add<TH2>("tracks/alpha/h2AlphaVspTNSigmaTPC", "NSigmaTPC(#alpha) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
      histos.add<TH2>("tracks/alpha/h2antiAlphaVspTNSigmaTPC", "NSigmaTPC(#bar{#alpha}) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
    }
    if (enableNucleiHardCut) {

      if (enablePr) {
        histos.add<TH2>("tracks/proton/hc/h2TPCsignVsTPCmomentumProton_hard", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
        histos.add<TH2>("tracks/proton/hc/h2TPCsignVsTPCmomentumantiProton_hard", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
      }
      if (enableDe) {
        histos.add<TH2>("tracks/deuteron/hc/h2TPCsignVsTPCmomentumDeuteron_hard", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
        histos.add<TH2>("tracks/deuteron/hc/h2TPCsignVsTPCmomentumantiDeuteron_hard", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
      }

      if (enableTr) {
        histos.add<TH2>("tracks/triton/hc/h2TPCsignVsTPCmomentumTriton_hard", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
        histos.add<TH2>("tracks/triton/hc/h2TPCsignVsTPCmomentumantiTriton_hard", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
      }

      if (enableHe) {
        histos.add<TH2>("tracks/helium/hc/h2TPCsignVsTPCmomentumHelium_hard", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
        histos.add<TH2>("tracks/helium/hc/h2TPCsignVsTPCmomentumantiHelium_hard", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{250, 0.f, 5.f}, {dedxAxis}});
      }
      if (doTOFplots) {
        if (enablePr) {
          histos.add<TH2>("tracks/proton/hc/h2TOFmass2ProtonVsPt_hard", "#Delta M^{2} (p) | Hard Cut vs #it{p}_{T}; #Delta M^{2} (p); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massPrAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/proton/hc/h2TOFmass2antiProtonVsPt_hard", "#Delta M^{2} (#bar{p}) | Hard Cut vs #it{p}_{T}; #Delta M^{2} (#bar{p}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massPrAxis}, {250, 0., 5.}});
        }
        if (enableDe) {
          histos.add<TH2>("tracks/deuteron/hc/h2TOFmass2DeuteronVsPt_hard", "#Delta M^{2} (d) | Hard Cut vs #it{p}_{T}; #Delta M^{2} (d); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massDeAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/deuteron/hc/h2TOFmass2antiDeuteronVsPt_hard", "#Delta M^{2} (#bar{d}) | Hard Cut vs #it{p}_{T}; #Delta M^{2} (#bar{d}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massDeAxis}, {250, 0., 5.}});
        }
        if (enableTr) {
          histos.add<TH2>("tracks/triton/hc/h2TOFmass2TritonVsPt_hard", "#Delta M^{2} (t) | Hard Cut vs #it{p}_{T}; #Delta M^{2} (t); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massTrAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/triton/hc/h2TOFmass2antiTritonVsPt_hard", "#Delta M^{2} (#bar{t}) | Hard Cut vs #it{p}_{T}; #Delta M^{2} (#bar{t}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massTrAxis}, {250, 0., 5.}});
        }
        if (enableHe) {
          histos.add<TH2>("tracks/helium/hc/h2TOFmass2HeliumVsPt_hard", "#Delta M^{2} (He) | Hard Cut vs #it{p}_{T}; #Delta M^{2} (He); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massHeAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/helium/hc/h2TOFmass2antiHeliumVsPt_hard", "#Delta M^{2} (#bar{He}) | Hard Cut vs #it{p}_{T}; #Delta M^{2} (#bar{He}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massHeAxis}, {250, 0., 5.}});
        }
      }
    }

    // TOF plots
    if (doTOFplots) {
      // TOF beta histograms
      if (enablePr) {
        histos.add<TH2>("tracks/proton/h2ProtonTOFbetaVsP", "TOF #beta (p) vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TOF #beta (p)", HistType::kTH2F, {{250, 0.f, 5.f}, {betaAxis}});
        histos.add<TH2>("tracks/proton/h2antiProtonTOFbetaVsP", "TOF #beta (#bar{p}) vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TOF #beta (#bar{p})", HistType::kTH2F, {{250, 0.f, 5.f}, {betaAxis}});
      }
      if (enableDe) {
        histos.add<TH2>("tracks/deuteron/h2DeuteronTOFbetaVsP", "TOF #beta (d) vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TOF #beta (d)", HistType::kTH2F, {{250, 0.f, 5.f}, {betaAxis}});
        histos.add<TH2>("tracks/deuteron/h2antiDeuteronTOFbetaVsP", "TOF #beta (#bar{d}) vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TOF #beta (#bar{d})", HistType::kTH2F, {{250, 0.f, 5.f}, {betaAxis}});
      }
      if (enableTr) {
        histos.add<TH2>("tracks/triton/h2TritonTOFbetaVsP", "TOF #beta (t) vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TOF #beta (t)", HistType::kTH2F, {{250, 0.f, 5.f}, {betaAxis}});
        histos.add<TH2>("tracks/triton/h2antiTritonTOFbetaVsP", "TOF #beta (#bar{t}) vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TOF #beta (#bar{t})", HistType::kTH2F, {{250, 0.f, 5.f}, {betaAxis}});
      }
      if (enableHe) {
        histos.add<TH2>("tracks/helium/h2HeliumTOFbetaVsP", "TOF #beta (He) vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TOF #beta (He)", HistType::kTH2F, {{250, 0.f, 5.f}, {betaAxis}});
        histos.add<TH2>("tracks/helium/h2antiHeliumTOFbetaVsP", "TOF #beta (#bar{He}) vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TOF #beta (#bar{He})", HistType::kTH2F, {{250, 0.f, 5.f}, {betaAxis}});
      }
      if (enableExpSignalTOF) {
        //  TOFExpSignal histograms
        if (enablePr) {
          histos.add<TH2>("tracks/proton/h2ProtonTOFExpSignalDiffVsPt", "TOF t - t_{exp}(p) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (p)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
          histos.add<TH2>("tracks/proton/h2antiProtonTOFExpSignalDiffVsPt", "TOF t - t_{exp}(#bar{p}) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{p})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
          histos.add<TH2>("tracks/proton/h2ProtonTOFExpSignalDiffVsPtCut", "TOF t - t_{exp}(p) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (p)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
          histos.add<TH2>("tracks/proton/h2antiProtonTOFExpSignalDiffVsPtCut", "TOF t - t_{exp}(#bar{p}) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{p})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
        }
        if (enableDe) {
          histos.add<TH2>("tracks/deuteron/h2DeuteronTOFExpSignalDiffVsPt", "TOF t - t_{exp}(d) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (d)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
          histos.add<TH2>("tracks/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt", "TOF t - t_{exp}(#bar{d}) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{d})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
          histos.add<TH2>("tracks/deuteron/h2DeuteronTOFExpSignalDiffVsPtCut", "TOF t - t_{exp}(d) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (d)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
          histos.add<TH2>("tracks/deuteron/h2antiDeuteronTOFExpSignalDiffVsPtCut", "TOF t - t_{exp}(#bar{d}) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{d})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
        }
        if (enableHe) {
          histos.add<TH2>("tracks/helium/h2HeliumTOFExpSignalDiffVsPt", "TOF t - t_{exp}(He) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (He)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
          histos.add<TH2>("tracks/helium/h2antiHeliumTOFExpSignalDiffVsPt", "TOF t - t_{exp}(#bar{He}) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{He})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
          histos.add<TH2>("tracks/helium/h2HeliumTOFExpSignalDiffVsPtCut", "TOF t - t_{exp}(He) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (He)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
          histos.add<TH2>("tracks/helium/h2antiHeliumTOFExpSignalDiffVsPtCut", "TOF t - t_{exp}(#bar{He}}) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{He})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
        }
      }

      // NSigmaTOF histograms
      histos.add<TH2>("tracks/pion/h2PionVspTNSigmaTOF", "NSigmaTOF(pi) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
      histos.add<TH2>("tracks/kaon/h2KaonVspTNSigmaTOF", "NSigmaTOF(Ka) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
      if (enablePr) {
        histos.add<TH2>("tracks/proton/h2ProtonVspTNSigmaTOF", "NSigmaTOF(p) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
        histos.add<TH2>("tracks/proton/h2antiProtonVspTNSigmaTOF", "NSigmaTOF(#bar{p}) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
      }
      if (enableDe) {
        histos.add<TH2>("tracks/deuteron/h2DeuteronVspTNSigmaTOF", "NSigmaTOF(d) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
        histos.add<TH2>("tracks/deuteron/h2antiDeuteronVspTNSigmaTOF", "NSigmaTOF(#bar{d}) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
      }
      if (enableHe) {
        histos.add<TH2>("tracks/helium/h2HeliumVspTNSigmaTOF", "NSigmaTOF(He) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
        histos.add<TH2>("tracks/helium/h2antiHeliumVspTNSigmaTOF", "NSigmaTOF(#bar{He}) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
      }
      if (enableDebug) {
        // NSigmaTPC vs NSigmaTOF histograms
        if (enablePr) {
          histos.add<TH3>("tracks/proton/h3ProtonNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC (p) vs NSigmaTOF(p); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{100, -20, 20}, {100, -20, 20}, {ptAxis}});
          histos.add<TH3>("tracks/proton/h3antiProtonNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC (#bar{p}) vs NSigmaTOF(#bar{p}); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{100, -20, 20}, {100, -20, 20}, {ptAxis}});
        }
        if (enableDe) {
          histos.add<TH3>("tracks/deuteron/h3DeuteronNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC(d) vs NSigmaTOF(d); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{100, -20, 20}, {100, -20, 20}, {ptAxis}});
          histos.add<TH3>("tracks/deuteron/h3antiDeuteronNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC (#bar{d}) vs NSigmaTOF(#bar{d}); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{100, -20, 20}, {100, -20, 20}, {ptAxis}});
        }
      }
      // TOF mass histograms
      histos.add<TH2>("tracks/h2TOFmassVsPt", "h2TOFmassVsPt; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
      if (enablePr) {
        histos.add<TH2>("tracks/proton/h2TOFmassProtonVsPt", "h2TOFmassProtonVsPt; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
        histos.add<TH2>("tracks/proton/h2TOFmassantiProtonVsPt", "h2TOFmassantiProtonVsPt; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
        if (enableBetaCut) {
          histos.add<TH2>("tracks/proton/h2TOFmassProtonVsPt_BetaCut", "h2TOFmassProtonVsPt_BetaCut; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
          histos.add<TH2>("tracks/proton/h2TOFmassantiProtonVsPt_BetaCut", "h2TOFmassantiProtonVsPt_BetaCut; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
        }
      }
      if (enableDe) {
        histos.add<TH2>("tracks/deuteron/h2TOFmassDeuteronVsPt", "h2TOFmassDeuteronVsPt; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
        histos.add<TH2>("tracks/deuteron/h2TOFmassantiDeuteronVsPt", "h2TOFmassantiDeuteronVsPt; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
        if (enableBetaCut) {
          histos.add<TH2>("tracks/deuteron/h2TOFmassDeuteronVsPt_BetaCut", "h2TOFmassDeuteronVsPt_BetaCut; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
          histos.add<TH2>("tracks/deuteron/h2TOFmassantiDeuteronVsPt_BetaCut", "h2TOFmassantiDeuteronVsPt_BetaCut; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
        }
      }
      if (enableTr) {
        histos.add<TH2>("tracks/triton/h2TOFmassTritonVsPt", "h2TOFmassTritonVsPt; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
        histos.add<TH2>("tracks/triton/h2TOFmassantiTritonVsPt", "h2TOFmassantiTritonVsPt; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
        if (enableBetaCut) {
          histos.add<TH2>("tracks/triton/h2TOFmassTritonVsPt_BetaCut", "h2TOFmassTritonVsPt_BetaCut; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
          histos.add<TH2>("tracks/triton/h2TOFmassantiTritonVsPt_BetaCut", "h2TOFmassantiTritonVsPt_BetaCut; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
        }
      }
      if (enableHe) {
        histos.add<TH2>("tracks/helium/h2TOFmassHeliumVsPt", "h2TOFmassHeliumVsPt; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
        histos.add<TH2>("tracks/helium/h2TOFmassantiHeliumVsPt", "h2TOFmassantiHeliumVsPt; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
        if (enableBetaCut) {
          histos.add<TH2>("tracks/helium/h2TOFmassHeliumVsPt_BetaCut", "h2TOFmassHeliumVsPt_BetaCut; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
          histos.add<TH2>("tracks/helium/h2TOFmassantiHeliumVsPt_BetaCut", "h2TOFmassantiHeliumVsPt_BetaCut; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
        }
        if (rapDebug) {
          histos.add<TH2>("tracks/helium/h2TOFmassHeliumVsPt_NORAPIDITYCUT", "h2TOFmassHeliumVsPt_NORAPIDITYCUT; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
          histos.add<TH2>("tracks/helium/h2TOFmassantiHeliumVsPt_NORAPIDITYCUT", "h2TOFmassantiHeliumVsPt_NORAPIDITYCUT; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{320, 0.4, 4.}, {250, 0., 5.}});
        }
      }
      // TOF mass squared histograms
      if (enablePr) {
        histos.add<TH2>("tracks/proton/h2TOFmass2ProtonVsPt", "#Delta M^{2} (p) vs #it{p}_{T}; #Delta M^{2} (p); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massPrAxis}, {250, 0., 5.}});
        histos.add<TH2>("tracks/proton/h2TOFmass2antiProtonVsPt", "#Delta M^{2} (#bar{p}) vs #it{p}_{T}; #Delta M^{2} (#bar{p}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massPrAxis}, {250, 0., 5.}});
        if (enableBetaCut) {
          histos.add<TH2>("tracks/proton/h2TOFmass2ProtonVsPt_BetaCut", "#Delta M^{2} (p) vs #it{p}_{T}; #Delta M^{2} (p); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massPrAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/proton/h2TOFmass2antiProtonVsPt_BetaCut", "#Delta M^{2} (#bar{p}) vs #it{p}_{T}; #Delta M^{2} (#bar{p}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massPrAxis}, {250, 0., 5.}});
        }
      }
      if (enableDe) {
        histos.add<TH2>("tracks/deuteron/h2TOFmass2DeuteronVsPt", "#Delta M^{2} (d) vs #it{p}_{T}; #Delta M^{2} (d); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massDeAxis}, {250, 0., 5.}});
        histos.add<TH2>("tracks/deuteron/h2TOFmass2antiDeuteronVsPt", "#Delta M^{2} (#bar{d}) vs #it{p}_{T}; #Delta M^{2} (#bar{d}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massDeAxis}, {250, 0., 5.}});
        if (enableBetaCut) {
          histos.add<TH2>("tracks/deuteron/h2TOFmass2DeuteronVsPt_BetaCut", "#Delta M^{2} (d) vs #it{p}_{T}; #Delta M^{2} (d); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massDeAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/deuteron/h2TOFmass2antiDeuteronVsPt_BetaCut", "#Delta M^{2} (#bar{d}) vs #it{p}_{T}; #Delta M^{2} (#bar{d}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massDeAxis}, {250, 0., 5.}});
        }
      }
      if (enableTr) {
        histos.add<TH2>("tracks/triton/h2TOFmass2TritonVsPt", "#Delta M^{2} (t) vs #it{p}_{T}; #Delta M^{2} (t); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massTrAxis}, {250, 0., 5.}});
        histos.add<TH2>("tracks/triton/h2TOFmass2antiTritonVsPt", "#Delta M^{2} (#bar{t}) vs #it{p}_{T}; #Delta M^{2} (#bar{t}; #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massTrAxis}, {250, 0., 5.}});
        if (enableBetaCut) {
          histos.add<TH2>("tracks/triton/h2TOFmass2TritonVsPt_BetaCut", "#Delta M^{2} (t) vs #it{p}_{T}; #Delta M^{2} (t); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massTrAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/triton/h2TOFmass2antiTritonVsPt_BetaCut", "#Delta M^{2} (#bar{t}) vs #it{p}_{T}; #Delta M^{2} (#bar{t}; #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massTrAxis}, {250, 0., 5.}});
        }
      }
      if (enableHe) {
        histos.add<TH2>("tracks/helium/h2TOFmass2antiHeliumVsPt", "#Delta M^{2} (#bar{He}) vs #it{p}_{T}; #Delta M^{2} (#bar{He}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massHeAxis}, {250, 0., 5.}});
        histos.add<TH2>("tracks/helium/h2TOFmass2HeliumVsPt", "#Delta M^{2} (He) vs #it{p}_{T}; #Delta M^{2} (He); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massHeAxis}, {250, 0., 5.}});
        if (enableBetaCut) {
          histos.add<TH2>("tracks/helium/h2TOFmass2antiHeliumVsPt_BetaCut", "#Delta M^{2} (#bar{He}) vs #it{p}_{T}; #Delta M^{2} (#bar{He}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massHeAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/helium/h2TOFmass2HeliumVsPt_BetaCut", "#Delta M^{2} (He) vs #it{p}_{T}; #Delta M^{2} (He); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massHeAxis}, {250, 0., 5.}});
        }
      }
      // TOF EvTime Splitting plots
      if (enableEvTimeSplitting) {
        //  Bethe-Bloch TPC distribution - TOF EvTime Splitted
        histos.add<TH2>("tracks/evtime/fill/h2TPCsignVsTPCmomentum", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{500, -5.f, 5.f}, {dedxAxis}});
        histos.add<TH2>("tracks/evtime/tof/h2TPCsignVsTPCmomentum", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{500, -5.f, 5.f}, {dedxAxis}});
        histos.add<TH2>("tracks/evtime/ft0/h2TPCsignVsTPCmomentum", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{500, -5.f, 5.f}, {dedxAxis}});
        histos.add<TH2>("tracks/evtime/ft0tof/h2TPCsignVsTPCmomentum", "TPC <-dE/dX> vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TPC <-dE/dx> (a.u.)", HistType::kTH2F, {{500, -5.f, 5.f}, {dedxAxis}});

        // Beta vs pT TOF distribution - TOF EvTime Splitted
        histos.add<TH2>("tracks/evtime/fill/h2TOFbetaVsP", "TOF #beta vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TOF #beta", HistType::kTH2F, {{500, -5.f, 5.f}, {betaAxis}});
        histos.add<TH2>("tracks/evtime/tof/h2TOFbetaVsP", "TOF #beta vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TOF #beta", HistType::kTH2F, {{500, -5.f, 5.f}, {betaAxis}});
        histos.add<TH2>("tracks/evtime/ft0/h2TOFbetaVsP", "TOF #beta vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TOF #beta", HistType::kTH2F, {{500, -5.f, 5.f}, {betaAxis}});
        histos.add<TH2>("tracks/evtime/ft0tof/h2TOFbetaVsP", "TOF #beta vs #it{p}/Z; Signed #it{p} (GeV/#it{c}); TOF #beta", HistType::kTH2F, {{500, -5.f, 5.f}, {betaAxis}});

        if (enableExpSignalTOF) {
          //  TOFExpSignal histograms - TOF EvTime Splitted
          if (enablePr) {
            histos.add<TH2>("tracks/evtime/fill/proton/h2ProtonTOFExpSignalDiffVsPt", "TOF t - t_{exp}(p) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (p)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("tracks/evtime/fill/proton/h2antiProtonTOFExpSignalDiffVsPt", "TOF t - t_{exp}(#bar{p}) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{p})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("tracks/evtime/tof/proton/h2ProtonTOFExpSignalDiffVsPt", "TOF t - t_{exp}(p) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (p)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("tracks/evtime/tof/proton/h2antiProtonTOFExpSignalDiffVsPt", "TOF t - t_{exp}(#bar{p}) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{p})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});

            histos.add<TH2>("tracks/evtime/ft0/proton/h2ProtonTOFExpSignalDiffVsPt", "TOF t - t_{exp}(p) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (p)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("tracks/evtime/ft0/proton/h2antiProtonTOFExpSignalDiffVsPt", "TOF t - t_{exp}(#bar{p}) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{p})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("tracks/evtime/ft0tof/proton/h2ProtonTOFExpSignalDiffVsPt", "TOF t - t_{exp}(p) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (p)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("tracks/evtime/ft0tof/proton/h2antiProtonTOFExpSignalDiffVsPt", "TOF t - t_{exp}(#bar{p}) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{p})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
          }
          if (enableDe) {
            histos.add<TH2>("tracks/evtime/fill/deuteron/h2DeuteronTOFExpSignalDiffVsPt", "TOF t - t_{exp}(d) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (d)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("tracks/evtime/fill/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt", "TOF t - t_{exp}(#bar{d}) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{d})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});

            histos.add<TH2>("tracks/evtime/tof/deuteron/h2DeuteronTOFExpSignalDiffVsPt", "TOF t - t_{exp}(d) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (d)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("tracks/evtime/tof/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt", "TOF t - t_{exp}(#bar{d}) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{d})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});

            histos.add<TH2>("tracks/evtime/ft0/deuteron/h2DeuteronTOFExpSignalDiffVsPt", "TOF t - t_{exp}(d) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (d)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("tracks/evtime/ft0/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt", "TOF t - t_{exp}(#bar{d}) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{d})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});

            histos.add<TH2>("tracks/evtime/ft0tof/deuteron/h2DeuteronTOFExpSignalDiffVsPt", "TOF t - t_{exp}(d) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (d)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("tracks/evtime/ft0tof/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt", "TOF t - t_{exp}(#bar{d}) vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{d})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
          }
        }

        // NSigmaTOF histograms - TOF EvTime Splitted
        if (enablePr) {
          histos.add<TH2>("tracks/evtime/fill/proton/h2ProtonVspTNSigmaTOF", "NSigmaTOF(p) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("tracks/evtime/fill/proton/h2antiProtonVspTNSigmaTOF", "NSigmaTOF(#bar{p}) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("tracks/evtime/tof/proton/h2ProtonVspTNSigmaTOF", "NSigmaTOF(#bar{p}) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("tracks/evtime/tof/proton/h2antiProtonVspTNSigmaTOF", "NSigmaTOF(#bar{p}) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("tracks/evtime/ft0/proton/h2ProtonVspTNSigmaTOF", "NSigmaTOF(p) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("tracks/evtime/ft0/proton/h2antiProtonVspTNSigmaTOF", "NSigmaTOF(#bar{p}) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("tracks/evtime/ft0tof/proton/h2ProtonVspTNSigmaTOF", "NSigmaTOF(p) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("tracks/evtime/ft0tof/proton/h2antiProtonVspTNSigmaTOF", "NSigmaTOF(#bar{p}) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
        }
        if (enableDe) {
          histos.add<TH2>("tracks/evtime/fill/deuteron/h2DeuteronVspTNSigmaTOF", "NSigmaTOF(d) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("tracks/evtime/fill/deuteron/h2antiDeuteronVspTNSigmaTOF", "NSigmaTOF(#bar{d}) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("tracks/evtime/tof/deuteron/h2DeuteronVspTNSigmaTOF", "NSigmaTOF(d) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("tracks/evtime/tof/deuteron/h2antiDeuteronVspTNSigmaTOF", "NSigmaTOF(#bar{d}) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("tracks/evtime/ft0/deuteron/h2DeuteronVspTNSigmaTOF", "NSigmaTOF(d) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("tracks/evtime/ft0/deuteron/h2antiDeuteronVspTNSigmaTOF", "NSigmaTOF(#bar{d}) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("tracks/evtime/ft0tof/deuteron/h2DeuteronVspTNSigmaTOF", "NSigmaTOF(d) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("tracks/evtime/ft0tof/deuteron/h2antiDeuteronVspTNSigmaTOF", "NSigmaTOF(#bar{d}) vs pT; #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
        }

        // NSigmaTPC vs NSigmaTOF histograms - TOF EvTime Splitted
        if (enablePr) {
          histos.add<TH3>("tracks/evtime/fill/proton/h3ProtonNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC (p) vs NSigmaTOF(p); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{200, -20, 20}, {200, -20, 20}, {ptAxis}});
          histos.add<TH3>("tracks/evtime/fill/proton/h3antiProtonNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC(#bar{p}) vs NSigmaTOF(#bar{p}); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{200, -20, 20}, {200, -20, 20}, {ptAxis}});
          histos.add<TH3>("tracks/evtime/tof/proton/h3ProtonNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC (p) vs NSigmaTOF(p); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{200, -20, 20}, {200, -20, 20}, {ptAxis}});
          histos.add<TH3>("tracks/evtime/tof/proton/h3antiProtonNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC(#bar{p}) vs NSigmaTOF(#bar{p}); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{200, -20, 20}, {200, -20, 20}, {ptAxis}});
          histos.add<TH3>("tracks/evtime/ft0/proton/h3ProtonNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC (p) vs NSigmaTOF(p); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{200, -20, 20}, {200, -20, 20}, {ptAxis}});
          histos.add<TH3>("tracks/evtime/ft0/proton/h3antiProtonNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC(#bar{p}) vs NSigmaTOF(#bar{p}); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{200, -20, 20}, {200, -20, 20}, {ptAxis}});
          histos.add<TH3>("tracks/evtime/ft0tof/proton/h3ProtonNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC (p) vs NSigmaTOF(p); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{200, -20, 20}, {200, -20, 20}, {ptAxis}});
          histos.add<TH3>("tracks/evtime/ft0tof/proton/h3antiProtonNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC(#bar{p}) vs NSigmaTOF(#bar{p}); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{200, -20, 20}, {200, -20, 20}, {ptAxis}});
        }
        if (enableDe) {
          histos.add<TH3>("tracks/evtime/fill/deuteron/h3DeuteronNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC(d) vs NSigmaTOF(d); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{200, -20, 20}, {200, -20, 20}, {ptAxis}});
          histos.add<TH3>("tracks/evtime/fill/deuteron/h3antiDeuteronNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC(#bar{d}) vs NSigmaTOF(#bar{p}); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{200, -20, 20}, {200, -20, 20}, {ptAxis}});
          histos.add<TH3>("tracks/evtime/tof/deuteron/h3DeuteronNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC(d) vs NSigmaTOF(d); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{200, -20, 20}, {200, -20, 20}, {ptAxis}});
          histos.add<TH3>("tracks/evtime/tof/deuteron/h3antiDeuteronNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC(#bar{d}) vs NSigmaTOF(#bar{p}); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{200, -20, 20}, {200, -20, 20}, {ptAxis}});
          histos.add<TH3>("tracks/evtime/ft0/deuteron/h3DeuteronNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC(d) vs NSigmaTOF(d); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{200, -20, 20}, {200, -20, 20}, {ptAxis}});
          histos.add<TH3>("tracks/evtime/ft0/deuteron/h3antiDeuteronNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC(#bar{d}) vs NSigmaTOF(#bar{p}); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{200, -20, 20}, {200, -20, 20}, {ptAxis}});
          histos.add<TH3>("tracks/evtime/ft0tof/deuteron/h3DeuteronNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC(d) vs NSigmaTOF(d); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{200, -20, 20}, {200, -20, 20}, {ptAxis}});
          histos.add<TH3>("tracks/evtime/ft0tof/deuteron/h3antiDeuteronNSigmaTPCvsNSigmaTOFvsPt", "NSigmaTPC(#bar{d}) vs NSigmaTOF(#bar{p}); NSigmaTPC; NSigmaTOF; #it{p}_{T} (GeV/#it{c})", HistType::kTH3F, {{200, -20, 20}, {200, -20, 20}, {ptAxis}});
        }

        // TOF mass histograms - TOF EvTime Splitted
        histos.add<TH2>("tracks/evtime/fill/h2TOFmassVsPt", "h2TOFmassVsPt; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{80, 0.4, 2.}, {250, 0., 5.}});
        histos.add<TH2>("tracks/evtime/tof/h2TOFmassVsPt", "h2TOFmassVsPt; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{80, 0.4, 2.}, {250, 0., 5.}});
        histos.add<TH2>("tracks/evtime/ft0/h2TOFmassVsPt", "h2TOFmassVsPt; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{80, 0.4, 2.}, {250, 0., 5.}});
        histos.add<TH2>("tracks/evtime/ft0tof/h2TOFmassVsPt", "h2TOFmassVsPt; TOFmass; #it{p}_{T} (GeV)", HistType::kTH2F, {{80, 0.4, 2.}, {250, 0., 5.}});

        // TOF mass squared histograms - TOF EvTime Splitted
        if (enablePr) {
          histos.add<TH2>("tracks/evtime/fill/proton/h2TOFmass2ProtonVsPt", "#Delta M^{2} (p) vs #it{p}_{T}; #Delta M^{2} (p); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massPrAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/evtime/fill/proton/h2TOFmass2antiProtonVsPt", "#Delta M^{2} (#bar{p}) vs #it{p}_{T}; #Delta M^{2} (#bar{p}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massPrAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/evtime/tof/proton/h2TOFmass2ProtonVsPt", "#Delta M^{2} (p) vs #it{p}_{T}; #Delta M^{2} (p); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massPrAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/evtime/tof/proton/h2TOFmass2antiProtonVsPt", "#Delta M^{2} (#bar{p}) vs #it{p}_{T}; #Delta M^{2} (#bar{p}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massPrAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/evtime/ft0/proton/h2TOFmass2ProtonVsPt", "#Delta M^{2} (p) vs #it{p}_{T}; #Delta M^{2} (p); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massPrAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/evtime/ft0/proton/h2TOFmass2antiProtonVsPt", "#Delta M^{2} (#bar{p}) vs #it{p}_{T}; #Delta M^{2} (#bar{p}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massPrAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/evtime/ft0tof/proton/h2TOFmass2ProtonVsPt", "#Delta M^{2} (p) vs #it{p}_{T}; #Delta M^{2} (p); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massPrAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/evtime/ft0tof/proton/h2TOFmass2antiProtonVsPt", "#Delta M^{2} (#bar{p}) vs #it{p}_{T}; #Delta M^{2} (#bar{p}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massPrAxis}, {250, 0., 5.}});
        }
        if (enableDe) {
          histos.add<TH2>("tracks/evtime/fill/deuteron/h2TOFmass2DeuteronVsPt", "#Delta M^{2} (d) vs #it{p}_{T}; #Delta M^{2} (d); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massDeAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/evtime/tof/deuteron/h2TOFmass2DeuteronVsPt", "#Delta M^{2} (d) vs #it{p}_{T}; #Delta M^{2} (d); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massDeAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/evtime/ft0/deuteron/h2TOFmass2DeuteronVsPt", "#Delta M^{2} (d) vs #it{p}_{T}; #Delta M^{2} (d); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massDeAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/evtime/ft0tof/deuteron/h2TOFmass2DeuteronVsPt", "#Delta M^{2} (d) vs #it{p}_{T}; #Delta M^{2} (d); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massDeAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/evtime/fill/deuteron/h2TOFmass2antiDeuteronVsPt", "#Delta M^{2} (#bar{d}) vs #it{p}_{T}; #Delta M^{2} (#bar{d}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massDeAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/evtime/tof/deuteron/h2TOFmass2antiDeuteronVsPt", "#Delta M^{2} (#bar{d}) vs #it{p}_{T}; #Delta M^{2} (#bar{d}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massDeAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/evtime/ft0/deuteron/h2TOFmass2antiDeuteronVsPt", "#Delta M^{2} (#bar{d}) vs #it{p}_{T}; #Delta M^{2} (#bar{d}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massDeAxis}, {250, 0., 5.}});
          histos.add<TH2>("tracks/evtime/ft0tof/deuteron/h2TOFmass2antiDeuteronVsPt", "#Delta M^{2} (#bar{d}) vs #it{p}_{T}; #Delta M^{2} (#bar{d}); #it{p}_{T} (GeV/#it{c})", HistType::kTH2F, {{massDeAxis}, {250, 0., 5.}});
        }
      }

      if (enableDebug) {
        histos.add<TH2>("debug/qa/h2TPCncrVsPtPos", "number of crossed rows in TPC vs Pt;  #it{p}_{T} (GeV/#it{c}); TPCncr", HistType::kTH2F, {{ptAxis}, {150, 60, 170}});
        histos.add<TH2>("debug/qa/h2TPCncrVsTPCsignalPos", "number of crossed rows in TPC vs Pt;  TPC <-dE/dX>; TPCncr", HistType::kTH2F, {dedxAxis, {150, 60, 170}});
        histos.add<TH1>("debug/qa/h1TPCncrLowPPos", "number of crossed rows in TPC (p<0.5 GeV/c); TPCncr; counts", HistType::kTH1F, {{150, 60, 170}});
        histos.add<TH1>("debug/qa/h1TPCncrMidPPos", "number of crossed rows in TPC (0.5<p<1.0 GeV/c); TPCncr; counts", HistType::kTH1F, {{150, 60, 170}});
        histos.add<TH1>("debug/qa/h1TPCncrHighPPos", "number of crossed rows in TPC (p>1.0 GeV/c); TPCncr; counts", HistType::kTH1F, {{150, 60, 170}});

        histos.add<TH2>("debug/qa/h2TPCncrVsPtNeg", "number of crossed rows in TPC vs Pt;  #it{p}_{T} (GeV/#it{c}); TPCncr", HistType::kTH2F, {{ptAxis}, {150, 60, 170}});
        histos.add<TH2>("debug/qa/h2TPCncrVsTPCsignalNeg", "number of crossed rows in TPC vs Pt;  TPC <-dE/dX>; TPCncr", HistType::kTH2F, {dedxAxis, {150, 60, 170}});
        histos.add<TH1>("debug/qa/h1TPCncrLowPNeg", "number of crossed rows in TPC (p<0.5 GeV/c); TPCncr; counts", HistType::kTH1F, {{150, 60, 170}});
        histos.add<TH1>("debug/qa/h1TPCncrMidPNeg", "number of crossed rows in TPC (0.5<p<1.0 GeV/c); TPCncr; counts", HistType::kTH1F, {{150, 60, 170}});
        histos.add<TH1>("debug/qa/h1TPCncrHighPNeg", "number of crossed rows in TPC (p>1.0 GeV/c); TPCncr; counts", HistType::kTH1F, {{150, 60, 170}});

        // Beta < 0.5
        //  NSigmasTPC histograms
        if (enablePr) {
          histos.add<TH2>("debug/evtime/fill/proton/h2ProtonVspTNSigmaTPC_BetaCut", "NSigmaTPC(p) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
          histos.add<TH2>("debug/evtime/fill/proton/h2antiProtonVspTNSigmaTPC_BetaCut", "NSigmaTPC(#bar{p}) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
          histos.add<TH2>("debug/evtime/tof/proton/h2ProtonVspTNSigmaTPC_BetaCut", "NSigmaTPC(p) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
          histos.add<TH2>("debug/evtime/tof/proton/h2antiProtonVspTNSigmaTPC_BetaCut", "NSigmaTPC(#bar{p}) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
          histos.add<TH2>("debug/evtime/ft0/proton/h2ProtonVspTNSigmaTPC_BetaCut", "NSigmaTPC(p) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
          histos.add<TH2>("debug/evtime/ft0/proton/h2antiProtonVspTNSigmaTPC_BetaCut", "NSigmaTPC(#bar{p}) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
          histos.add<TH2>("debug/evtime/ft0tof/roton/h2ProtonVspTNSigmaTPC_BetaCut", "NSigmaTPC(p) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
          histos.add<TH2>("debug/evtime/ft0tof/proton/h2antiProtonVspTNSigmaTPC_BetaCut", "NSigmaTPC(#bar{p}) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
        }
        if (enableDe) {
          histos.add<TH2>("debug/evtime/fill/deuteron/h2DeuteronVspTNSigmaTPC_BetaCut", "NSigmaTPC(d) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
          histos.add<TH2>("debug/evtime/fill/deuteron/h2antiDeuteronVspTNSigmaTPC_BetaCut", "NSigmaTPC(#bar{d}) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
          histos.add<TH2>("debug/evtime/tof/deuteron/h2DeuteronVspTNSigmaTPC_BetaCut", "NSigmaTPC(d) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
          histos.add<TH2>("debug/evtime/tof/deuteron/h2antiDeuteronVspTNSigmaTPC_BetaCut", "NSigmaTPC(#bar{d}) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
          histos.add<TH2>("debug/evtime/ft0/deuteron/h2DeuteronVspTNSigmaTPC_BetaCut", "NSigmaTPC(d) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
          histos.add<TH2>("debug/evtime/ft0/deuteron/h2antiDeuteronVspTNSigmaTPC_BetaCut", "NSigmaTPC(#bar{d}) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
          histos.add<TH2>("debug/evtime/ft0tof/deuteron/h2DeuteronVspTNSigmaTPC_BetaCut", "NSigmaTPC(d) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
          histos.add<TH2>("debug/evtime/ft0tof/deuteron/h2antiDeuteronVspTNSigmaTPC_BetaCut", "NSigmaTPC(#bar{d}) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTPC", HistType::kTH2F, {{ptAxis}, {SigmaTPCAxis}});
        }

        // NSigmaTOF histograms - TOF EvTime Splitted
        if (enablePr) {
          histos.add<TH2>("debug/evtime/fill/proton/h2ProtonVspTNSigmaTOF_BetaCut", "NSigmaTOF(p) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("debug/evtime/fill/proton/h2antiProtonVspTNSigmaTOF_BetaCut", "NSigmaTOF(#bar{p}) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("debug/evtime/tof/proton/h2ProtonVspTNSigmaTOF_BetaCut", "NSigmaTOF(p) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("debug/evtime/tof/proton/h2antiProtonVspTNSigmaTOF_BetaCut", "NSigmaTOF(#bar{p}) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("debug/evtime/ft0/proton/h2ProtonVspTNSigmaTOF_BetaCut", "NSigmaTOF(p) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("debug/evtime/ft0/proton/h2antiProtonVspTNSigmaTOF_BetaCut", "NSigmaTOF(#bar{p}) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("debug/evtime/ft0tof/proton/h2ProtonVspTNSigmaTOF_BetaCut", "NSigmaTOF(p) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("debug/evtime/ft0tof/proton/h2antiProtonVspTNSigmaTOF_BetaCut", "NSigmaTOF(#bar{p}) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
        }
        if (enableDe) {
          histos.add<TH2>("debug/evtime/fill/deuteron/h2DeuteronVspTNSigmaTOF_BetaCut", "NSigmaTOF(d) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("debug/evtime/fill/deuteron/h2antiDeuteronVspTNSigmaTOF_BetaCut", "NSigmaTOF(#bar{d}) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("debug/evtime/tof/deuteron/h2DeuteronVspTNSigmaTOF_BetaCut", "NSigmaTOF(d) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("debug/evtime/tof/deuteron/h2antiDeuteronVspTNSigmaTOF_BetaCut", "NSigmaTOF(#bar{d}) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("debug/evtime/ft0/deuteron/h2DeuteronVspTNSigmaTOF_BetaCut", "NSigmaTOF(d) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("debug/evtime/ft0/deuteron/h2antiDeuteronVspTNSigmaTOF_BetaCut", "NSigmaTOF(#bar{d}) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("debug/evtime/ft0tof/deuteron/h2DeuteronVspTNSigmaTOF_BetaCut", "NSigmaTOF(d) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
          histos.add<TH2>("debug/evtime/ft0tof/deuteron/h2antiDeuteronVspTNSigmaTOF_BetaCut", "NSigmaTOF(#bar{d}) vs pT (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); NSigmaTOF", HistType::kTH2F, {{ptAxis}, {SigmaTOFAxis}});
        }

        if (enableExpSignalTOF) {
          //  TOFExpSignal histograms - TOF EvTime Splitted
          if (enablePr) {
            histos.add<TH2>("debug/evtime/fill/proton/h2ProtonTOFExpSignalDiffVsPt_BetaCut", "TOF t - t_{exp}(p) vs #it{p}_{T} (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (p)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("debug/evtime/fill/proton/h2antiProtonTOFExpSignalDiffVsPt_BetaCut", "TOF t - t_{exp}(#bar{p}) vs #it{p}_{T} (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{p})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("debug/evtime/tof/proton/h2ProtonTOFExpSignalDiffVsPt_BetaCut", "TOF t - t_{exp}(p) vs #it{p}_{T} (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (p)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("debug/evtime/tof/proton/h2antiProtonTOFExpSignalDiffVsPt_BetaCut", "TOF t - t_{exp}(#bar{p}) vs #it{p}_{T} (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{p})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("debug/evtime/ft0/proton/h2ProtonTOFExpSignalDiffVsPt_BetaCut", "TOF t - t_{exp}(p) vs #it{p}_{T} (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (p)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("debug/evtime/ft0/proton/h2antiProtonTOFExpSignalDiffVsPt_BetaCut", "TOF t - t_{exp}(#bar{p}) vs #it{p}_{T} (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{p})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("debug/evtime/ft0tof/proton/h2ProtonTOFExpSignalDiffVsPt_BetaCut", "TOF t - t_{exp}(p) vs #it{p}_{T} (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (p)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("debug/evtime/ft0tof/proton/h2antiProtonTOFExpSignalDiffVsPt_BetaCut", "TOF t - t_{exp}(#bar{p}) vs #it{p}_{T} (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{p})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
          }
          if (enableDe) {
            histos.add<TH2>("debug/evtime/fill/deuteron/h2DeuteronTOFExpSignalDiffVsPt_BetaCut", "TOF t - t_{exp}(d) vs #it{p}_{T} (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (d)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("debug/evtime/fill/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt_BetaCut", "TOF t - t_{exp}(#bar{d}) vs #it{p}_{T} (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{d})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("debug/evtime/tof/deuteron/h2DeuteronTOFExpSignalDiffVsPt_BetaCut", "TOF t - t_{exp}(d) vs #it{p}_{T} (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (d)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("debug/evtime/tof/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt_BetaCut", "TOF t - t_{exp}(#bar{d}) vs #it{p}_{T} (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{d})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("debug/evtime/ft0/deuteron/h2DeuteronTOFExpSignalDiffVsPt_BetaCut", "TOF t - t_{exp}(d) vs #it{p}_{T} (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (d)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("debug/evtime/ft0/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt_BetaCut", "TOF t - t_{exp}(#bar{d}) vs #it{p}_{T} (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{d})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("debug/evtime/ft0tof/deuteron/h2DeuteronTOFExpSignalDiffVsPt_BetaCut", "TOF t - t_{exp}(d) vs #it{p}_{T} (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (d)", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
            histos.add<TH2>("debug/evtime/ft0tof/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt_BetaCut", "TOF t - t_{exp}(#bar{d}) vs #it{p}_{T} (#beta < 0.5); #it{p}_{T} (GeV/#it{c}); TOF t - t_{exp} (#bar{d})", HistType::kTH2F, {{ptAxis}, {2000, -25000, 25000}});
          }
        }
      }
    }

    if (!doprocessMCGen) {
      return;
    }
    // MC histograms  -   all, primary, sec. from weak decay, sec. from material
    histos.add("spectraGen/histGenVetxZ", "PosZ generated events", HistType::kTH1F, {{1500, -15.f, 15.f, "Vertex Z (cm)"}});

    histos.add("spectraGen/pion/histGenPtPion", "generated particles", HistType::kTH1F, {ptAxis});
    histos.add("spectraGen/pion/histGenPtPionPrim", "generated particles", HistType::kTH1F, {ptAxis});
    histos.add("spectraGen/pion/histGenPtPionSec", "generated particles", HistType::kTH1F, {ptAxis});
    histos.add("spectraGen/pion/histSecTransportPtPion", "generated particles", HistType::kTH1F, {ptAxis});

    histos.add("spectraGen/pion/histGenPtantiPion", "generated particles", HistType::kTH1F, {ptAxis});
    histos.add("spectraGen/pion/histGenPtantiPionPrim", "generated particles", HistType::kTH1F, {ptAxis});
    histos.add("spectraGen/pion/histGenPtantiPionSec", "generated particles", HistType::kTH1F, {ptAxis});
    histos.add("spectraGen/pion/histSecTransportPtantiPion", "generated particles", HistType::kTH1F, {ptAxis});

    histos.add("spectraGen/kaon/histGenPtKaon", "generated particles", HistType::kTH1F, {ptAxis});
    histos.add("spectraGen/kaon/histGenPtKaonPrim", "generated particles", HistType::kTH1F, {ptAxis});
    histos.add("spectraGen/kaon/histGenPtKaonSec", "generated particles", HistType::kTH1F, {ptAxis});
    histos.add("spectraGen/kaon/histSecTransportPtKaon", "generated particles", HistType::kTH1F, {ptAxis});

    histos.add("spectraGen/kaon/histGenPtantiKaon", "generated particles", HistType::kTH1F, {ptAxis});
    histos.add("spectraGen/kaon/histGenPtantiKaonPrim", "generated particles", HistType::kTH1F, {ptAxis});
    histos.add("spectraGen/kaon/histGenPtantiKaonSec", "generated particles", HistType::kTH1F, {ptAxis});
    histos.add("spectraGen/kaon/histSecTransportPtantiKaon", "generated particles", HistType::kTH1F, {ptAxis});

    if (enablePr) {
      histos.add("spectraGen/proton/histGenPtProton", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/proton/histGenPtProtonPrim", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/proton/histGenPtProtonSec", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/proton/histSecTransportPtProton", "generated particles", HistType::kTH1F, {ptAxis});

      histos.add("spectraGen/proton/histGenPtantiProton", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/proton/histGenPtantiProtonPrim", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/proton/histGenPtantiProtonSec", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/proton/histSecTransportPtantiProton", "generated particles", HistType::kTH1F, {ptAxis});
    }
    if (enableDe) {
      histos.add("spectraGen/deuteron/histGenPtD", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/deuteron/histGenPtDPrim", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/deuteron/histGenPtDSec", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/deuteron/histSecTransportPtD", "generated particles", HistType::kTH1F, {ptAxis});

      histos.add("spectraGen/deuteron/histGenPtantiD", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/deuteron/histGenPtantiDPrim", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/deuteron/histGenPtantiDSec", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/deuteron/histSecTransportPtantiD", "generated particles", HistType::kTH1F, {ptAxis});
    }
    if (enableTr) {
      histos.add("spectraGen/triton/histGenPtT", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/triton/histGenPtTPrim", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/triton/histGenPtTSec", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/triton/histSecTransportPtT", "generated particles", HistType::kTH1F, {ptAxis});

      histos.add("spectraGen/triton/histGenPtantiT", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/triton/histGenPtantiTPrim", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/triton/histGenPtantiTSec", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/triton/histSecTransportPtantiT", "generated particles", HistType::kTH1F, {ptAxis});
    }
    if (enableHe) {
      histos.add("spectraGen/helium/histGenPtHe", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/helium/histGenPtHePrim", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/helium/histGenPtHeSec", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/helium/histSecTransportPtHe", "generated particles", HistType::kTH1F, {ptAxis});

      histos.add("spectraGen/helium/histGenPtantiHe", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/helium/histGenPtantiHePrim", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/helium/histGenPtantiHeSec", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/helium/histSecTransportPtantiHe", "generated particles", HistType::kTH1F, {ptAxis});
    }
    if (enableAl) {
      histos.add("spectraGen/alpha/histGenPtAl", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/alpha/histGenPtAlPrim", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/alpha/histGenPtAlSec", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/alpha/histSecTransportPtAl", "generated particles", HistType::kTH1F, {ptAxis});

      histos.add("spectraGen/alpha/histGenPtantiAl", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/alpha/histGenPtantiAlPrim", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/alpha/histGenPtantiAlSec", "generated particles", HistType::kTH1F, {ptAxis});
      histos.add("spectraGen/alpha/histSecTransportPtantiAl", "generated particles", HistType::kTH1F, {ptAxis});
    }
    LOG(info) << "Histograms of LFNucleiBATask:";
    histos.print();
  }

  template <bool IsMC, bool IsFilteredData, typename CollisionType, typename TracksType, typename ParticleType>
  void fillHistograms(const CollisionType& event,
                      const TracksType& tracks,
                      const ParticleType& particles)
  {

    if constexpr (!IsFilteredData) {
      if (!event.sel8()) {
        return;
      }
      if (event.posZ() < cfgLowCutVertex || event.posZ() > cfgHighCutVertex) {
        return;
      }
    }

    float gamma = 0., massTOF = 0., hePt = 0.f;
    bool isTriton = kFALSE;
    bool deRapCut = kFALSE;
    bool heRapCut = kFALSE;

    // Event histos fill
    histos.fill(HIST("event/h1VtxZ"), event.posZ());
    if (enableDebug)
      histos.fill(HIST("event/h1CentV0M"), event.multFV0M());

    for (auto& track : tracks) {
      histos.fill(HIST("tracks/h1pT"), track.pt());
      histos.fill(HIST("tracks/h1p"), track.p());

      if constexpr (!IsFilteredData) {
        if (!track.isGlobalTrackWoDCA()) {
          continue;
        }
      }
      std::bitset<8> itsClusterMap = track.itsClusterMap();
      if (track.itsNCls() < cfgCutITSClusters)
        continue;
      if (track.tpcNClsCrossedRows() < cfgCutTPCXRows)
        continue;
      if (track.tpcNClsFound() < cfgCutTPCClusters)
        continue;

      switch (tritonSelConfig) {
        case 0:
          isTriton = std::abs(track.tpcNSigmaTr()) < nsigmaTPCTr;
          break;
        case 1:
          isTriton = (std::abs(track.tpcNSigmaTr()) < nsigmaTPCTr) && (std::abs(track.tpcNSigmaPr()) > 3) && (std::abs(track.tpcNSigmaKa()) > 5) && (std::abs(track.tpcNSigmaPi()) > 5);
          break;
      }

      switch (helium3Pt) {
        case 0:
          hePt = track.pt();
          break;
        case 1:
          hePt = 2.f * track.pt();
          break;
      }

      // debug on helium rapidity cut
      deRapCut = TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Deuteron))) < yCut;
      heRapCut = TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Helium3))) < yCut;

      // Tracks DCA histos fill
      if (makeDCABeforeCutPlots) {
        histos.fill(HIST("tracks/dca/before/hDCAxy"), track.dcaXY());
        histos.fill(HIST("tracks/dca/before/hDCAz"), track.dcaZ());
        histos.fill(HIST("tracks/dca/before/hDCAxyVsDCAz"), track.dcaXY(), track.dcaZ());
        histos.fill(HIST("tracks/dca/before/hDCAxyVsPt"), track.pt(), track.dcaXY());
        histos.fill(HIST("tracks/dca/before/hDCAzVsPt"), track.pt(), track.dcaZ());

        if (enablePr) {
          if (std::abs(track.tpcNSigmaPr()) < nsigmaTPCPr) {
            if (track.sign() > 0) {
              histos.fill(HIST("tracks/proton/dca/before/hDCAxyVsPtProton"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/proton/dca/before/hDCAzVsPtProton"), track.pt(), track.dcaZ());
            } else {
              histos.fill(HIST("tracks/proton/dca/before/hDCAxyVsPtantiProton"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/proton/dca/before/hDCAzVsPtantiProton"), track.pt(), track.dcaZ());
            }
          }
        }
        if (enableDe) {
          if (std::abs(track.tpcNSigmaDe()) < nsigmaTPCDe) {
            if (usenITSLayer && !itsClusterMap.test(nITSLayer))
              continue;
            if (track.sign() > 0) {
              histos.fill(HIST("tracks/deuteron/dca/before/hDCAxyVsPtDeuteron"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/deuteron/dca/before/hDCAzVsPtDeuteron"), track.pt(), track.dcaZ());
            } else {
              histos.fill(HIST("tracks/deuteron/dca/before/hDCAxyVsPtantiDeuteron"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/deuteron/dca/before/hDCAzVsPtantiDeuteron"), track.pt(), track.dcaZ());
            }
          }
        }
        if (enableTr) {
          if (isTriton) {
            if (track.sign() > 0) {
              histos.fill(HIST("tracks/triton/dca/before/hDCAxyVsPtTriton"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/triton/dca/before/hDCAzVsPtTriton"), track.pt(), track.dcaZ());
            } else {
              histos.fill(HIST("tracks/triton/dca/before/hDCAxyVsPtantiTriton"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/triton/dca/before/hDCAzVsPtantiTriton"), track.pt(), track.dcaZ());
            }
          }
        }
        if (enableHe) {
          if (std::abs(track.tpcNSigmaHe()) < nsigmaTPCHe) {
            if (track.sign() > 0) {
              histos.fill(HIST("tracks/helium/dca/before/hDCAxyVsPtHelium"), hePt, track.dcaXY());
              histos.fill(HIST("tracks/helium/dca/before/hDCAzVsPtHelium"), hePt, track.dcaZ());
            } else {
              histos.fill(HIST("tracks/helium/dca/before/hDCAxyVsPtantiHelium"), hePt, track.dcaXY());
              histos.fill(HIST("tracks/helium/dca/before/hDCAzVsPtantiHelium"), hePt, track.dcaZ());
            }
          }
        }
        if (enableAl) {
          if (std::abs(track.tpcNSigmaAl()) < nsigmaTPCAl) {
            if (track.sign() > 0) {
              histos.fill(HIST("tracks/alpha/dca/before/hDCAxyVsPtAlpha"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/alpha/dca/before/hDCAzVsPtAlpha"), track.pt(), track.dcaZ());
            } else {
              histos.fill(HIST("tracks/alpha/dca/before/hDCAxyVsPtantiAlpha"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/alpha/dca/before/hDCAzVsPtantiAlpha"), track.pt(), track.dcaZ());
            }
          }
        }

        if constexpr (IsMC) {
          bool isPhysPrim = false;
          bool isProdByGen = false;

          // PID
          int pdgCode = 0;
          if constexpr (IsFilteredData) {
            isPhysPrim = track.isPhysicalPrimary();
            isProdByGen = track.producedByGenerator();
            pdgCode = track.pdgCode();
          } else {
            if (!track.has_mcParticle()) {
              continue;
            }
            isPhysPrim = track.mcParticle().isPhysicalPrimary();
            isProdByGen = track.mcParticle().producedByGenerator();
            pdgCode = track.mcParticle().pdgCode();
          }
          switch (pdgCode) {
            case PDGProton:
              if (enablePr) {
                histos.fill(HIST("tracks/proton/dca/before/hDCAxyVsPtProtonTrue"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/proton/dca/before/hDCAzVsPtProtonTrue"), track.pt(), track.dcaZ());
                if (isPhysPrim) {
                  histos.fill(HIST("tracks/proton/dca/before/hDCAxyVsPtProtonTruePrim"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/proton/dca/before/hDCAzVsPtProtonTruePrim"), track.pt(), track.dcaZ());
                }
                if (!isPhysPrim && isProdByGen) {
                  histos.fill(HIST("tracks/proton/dca/before/hDCAxyVsPtProtonTrueSec"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/proton/dca/before/hDCAzVsPtProtonTrueSec"), track.pt(), track.dcaZ());
                }
                if (!isPhysPrim && !isProdByGen) {
                  histos.fill(HIST("tracks/proton/dca/before/hDCAxyVsPtProtonTrueTransport"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/proton/dca/before/hDCAzVsPtProtonTrueTransport"), track.pt(), track.dcaZ());
                }
              }
              break;
            case -PDGProton:
              if (enablePr) {
                histos.fill(HIST("tracks/proton/dca/before/hDCAxyVsPtantiProtonTrue"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/proton/dca/before/hDCAzVsPtantiProtonTrue"), track.pt(), track.dcaZ());

                if (isPhysPrim) {
                  histos.fill(HIST("tracks/proton/dca/before/hDCAxyVsPtantiProtonTruePrim"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/proton/dca/before/hDCAzVsPtantiProtonTruePrim"), track.pt(), track.dcaZ());
                }
                if (!isPhysPrim && isProdByGen) {
                  histos.fill(HIST("tracks/proton/dca/before/hDCAxyVsPtantiProtonTrueSec"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/proton/dca/before/hDCAzVsPtantiProtonTrueSec"), track.pt(), track.dcaZ());
                }
                if (!isPhysPrim && !isProdByGen) {
                  histos.fill(HIST("tracks/proton/dca/before/hDCAxyVsPtantiProtonTrueTransport"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/proton/dca/before/hDCAzVsPtantiProtonTrueTransport"), track.pt(), track.dcaZ());
                }
              }
              break;
            case PDGDeuteron:
              if (enableDe) {
                histos.fill(HIST("tracks/deuteron/dca/before/hDCAxyVsPtDeuteronTrue"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/deuteron/dca/before/hDCAzVsPtDeuteronTrue"), track.pt(), track.dcaZ());
                if (isPhysPrim) {
                  histos.fill(HIST("tracks/deuteron/dca/before/hDCAxyVsPtDeuteronTruePrim"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/deuteron/dca/before/hDCAzVsPtDeuteronTruePrim"), track.pt(), track.dcaZ());
                }
                if (!isPhysPrim && isProdByGen) {
                  histos.fill(HIST("tracks/deuteron/dca/before/hDCAxyVsPtDeuteronTrueSec"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/deuteron/dca/before/hDCAzVsPtDeuteronTrueSec"), track.pt(), track.dcaZ());
                }
                if (!isPhysPrim && !isProdByGen) {
                  histos.fill(HIST("tracks/deuteron/dca/before/hDCAxyVsPtDeuteronTrueTransport"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/deuteron/dca/before/hDCAzVsPtDeuteronTrueTransport"), track.pt(), track.dcaZ());
                }
              }
              break;
            case -PDGDeuteron:
              if (enableDe) {
                histos.fill(HIST("tracks/deuteron/dca/before/hDCAxyVsPtantiDeuteronTrue"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/deuteron/dca/before/hDCAzVsPtantiDeuteronTrue"), track.pt(), track.dcaZ());
                if (isPhysPrim) {
                  histos.fill(HIST("tracks/deuteron/dca/before/hDCAxyVsPtantiDeuteronTruePrim"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/deuteron/dca/before/hDCAzVsPtantiDeuteronTruePrim"), track.pt(), track.dcaZ());
                }
                if (!isPhysPrim && isProdByGen) {
                  histos.fill(HIST("tracks/deuteron/dca/before/hDCAxyVsPtantiDeuteronTrueSec"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/deuteron/dca/before/hDCAzVsPtantiDeuteronTrueSec"), track.pt(), track.dcaZ());
                }
                if (!isPhysPrim && !isProdByGen) {
                  histos.fill(HIST("tracks/deuteron/dca/before/hDCAxyVsPtantiDeuteronTrueTransport"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/deuteron/dca/before/hDCAzVsPtantiDeuteronTrueTransport"), track.pt(), track.dcaZ());
                }
              }
              break;
            case PDGTriton:
              if (enableTr) {
                histos.fill(HIST("tracks/triton/dca/before/hDCAxyVsPtTritonTrue"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/triton/dca/before/hDCAzVsPtTritonTrue"), track.pt(), track.dcaZ());
                if (isPhysPrim) {
                  histos.fill(HIST("tracks/triton/dca/before/hDCAxyVsPtTritonTruePrim"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/triton/dca/before/hDCAzVsPtTritonTruePrim"), track.pt(), track.dcaZ());
                }
                if (!isPhysPrim && isProdByGen) {
                  histos.fill(HIST("tracks/triton/dca/before/hDCAxyVsPtTritonTrueSec"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/triton/dca/before/hDCAzVsPtTritonTrueSec"), track.pt(), track.dcaZ());
                }
                if (!isPhysPrim && !isProdByGen) {
                  histos.fill(HIST("tracks/triton/dca/before/hDCAxyVsPtTritonTrueTransport"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/triton/dca/before/hDCAzVsPtTritonTrueTransport"), track.pt(), track.dcaZ());
                }
              }
              break;
            case -PDGTriton:
              if (enableTr) {
                histos.fill(HIST("tracks/triton/dca/before/hDCAxyVsPtantiTritonTrue"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/triton/dca/before/hDCAzVsPtantiTritonTrue"), track.pt(), track.dcaZ());
                if (isPhysPrim) {
                  histos.fill(HIST("tracks/triton/dca/before/hDCAxyVsPtantiTritonTruePrim"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/triton/dca/before/hDCAzVsPtantiTritonTruePrim"), track.pt(), track.dcaZ());
                }
                if (!isPhysPrim && isProdByGen) {
                  histos.fill(HIST("tracks/triton/dca/before/hDCAxyVsPtantiTritonTrueSec"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/triton/dca/before/hDCAzVsPtantiTritonTrueSec"), track.pt(), track.dcaZ());
                }
                if (!isPhysPrim && !isProdByGen) {
                  histos.fill(HIST("tracks/triton/dca/before/hDCAxyVsPtantiTritonTrueTransport"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/triton/dca/before/hDCAzVsPtantiTritonTrueTransport"), track.pt(), track.dcaZ());
                }
              }
              break;
            case PDGHelium:
              if (enableHe) {
                histos.fill(HIST("tracks/helium/dca/before/hDCAxyVsPtHeliumTrue"), hePt, track.dcaXY());
                histos.fill(HIST("tracks/helium/dca/before/hDCAzVsPtHeliumTrue"), hePt, track.dcaZ());
                if (isPhysPrim) {
                  histos.fill(HIST("tracks/helium/dca/before/hDCAxyVsPtHeliumTruePrim"), hePt, track.dcaXY());
                  histos.fill(HIST("tracks/helium/dca/before/hDCAzVsPtHeliumTruePrim"), hePt, track.dcaZ());
                }
                if (!isPhysPrim && isProdByGen) {
                  histos.fill(HIST("tracks/helium/dca/before/hDCAxyVsPtHeliumTrueSec"), hePt, track.dcaXY());
                  histos.fill(HIST("tracks/helium/dca/before/hDCAzVsPtHeliumTrueSec"), hePt, track.dcaZ());
                }
                if (!isPhysPrim && !isProdByGen) {
                  histos.fill(HIST("tracks/helium/dca/before/hDCAxyVsPtHeliumTrueTransport"), hePt, track.dcaXY());
                  histos.fill(HIST("tracks/helium/dca/before/hDCAzVsPtHeliumTrueTransport"), hePt, track.dcaZ());
                }
              }
              break;
            case -PDGHelium:
              if (enableHe) {
                histos.fill(HIST("tracks/helium/dca/before/hDCAxyVsPtantiHeliumTrue"), hePt, track.dcaXY());
                histos.fill(HIST("tracks/helium/dca/before/hDCAzVsPtantiHeliumTrue"), hePt, track.dcaZ());

                if (isPhysPrim) {
                  histos.fill(HIST("tracks/helium/dca/before/hDCAxyVsPtantiHeliumTruePrim"), hePt, track.dcaXY());
                  histos.fill(HIST("tracks/helium/dca/before/hDCAzVsPtantiHeliumTruePrim"), hePt, track.dcaZ());
                }
                if (!isPhysPrim && isProdByGen) {
                  histos.fill(HIST("tracks/helium/dca/before/hDCAxyVsPtantiHeliumTrueSec"), hePt, track.dcaXY());
                  histos.fill(HIST("tracks/helium/dca/before/hDCAzVsPtantiHeliumTrueSec"), hePt, track.dcaZ());
                }
                if (!isPhysPrim && !isProdByGen) {
                  histos.fill(HIST("tracks/helium/h1antiHelimSpectraTrueTransport"), hePt);
                  histos.fill(HIST("tracks/helium/dca/before/hDCAxyVsPtantiHeliumTrueTransport"), hePt, track.dcaXY());
                  histos.fill(HIST("tracks/helium/dca/before/hDCAzVsPtantiHeliumTrueTransport"), hePt, track.dcaZ());
                }
              }
              break;
            case PDGAlpha:
              if (enableAl) {
                histos.fill(HIST("tracks/alpha/dca/before/hDCAxyVsPtAlphaTrue"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/alpha/dca/before/hDCAzVsPtAlphaTrue"), track.pt(), track.dcaZ());
                if (isPhysPrim) {
                  histos.fill(HIST("tracks/alpha/dca/before/hDCAxyVsPtAlphaTruePrim"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/alpha/dca/before/hDCAzVsPtAlphaTruePrim"), track.pt(), track.dcaZ());
                }
                if (!isPhysPrim && isProdByGen) {
                  histos.fill(HIST("tracks/alpha/dca/before/hDCAxyVsPtAlphaTrueSec"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/alpha/dca/before/hDCAzVsPtAlphaTrueSec"), track.pt(), track.dcaZ());
                }
                if (!isPhysPrim && !isProdByGen) {
                  histos.fill(HIST("tracks/alpha/dca/before/hDCAxyVsPtAlphaTrueTransport"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/alpha/dca/before/hDCAzVsPtAlphaTrueTransport"), track.pt(), track.dcaZ());
                }
              }
              break;
            case -PDGAlpha:
              if (enableAl) {
                histos.fill(HIST("tracks/alpha/dca/before/hDCAxyVsPtantiAlphaTrue"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/alpha/dca/before/hDCAzVsPtantiAlphaTrue"), track.pt(), track.dcaZ());
                if (isPhysPrim) {
                  histos.fill(HIST("tracks/alpha/dca/before/hDCAxyVsPtantiAlphaTruePrim"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/alpha/dca/before/hDCAzVsPtantiAlphaTruePrim"), track.pt(), track.dcaZ());
                }
                if (!isPhysPrim && isProdByGen) {
                  histos.fill(HIST("tracks/alpha/dca/before/hDCAxyVsPtantiAlphaTrueSec"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/alpha/dca/before/hDCAzVsPtantiAlphaTrueSec"), track.pt(), track.dcaZ());
                }
                if (!isPhysPrim && !isProdByGen) {
                  histos.fill(HIST("tracks/alpha/dca/before/hDCAxyVsPtantiAlphaTrueTransport"), track.pt(), track.dcaXY());
                  histos.fill(HIST("tracks/alpha/dca/before/hDCAzVsPtantiAlphaTrueTransport"), track.pt(), track.dcaZ());
                }
              }
              break;
            default:
              break;
          }
        }
      }

      // DCA Cut
      if constexpr (!IsFilteredData) {
        if (!enableDCACustomCut) {
          if (!track.isGlobalTrack())
            continue;
        } else {
          if (!track.isGlobalTrackWoDCA())
            continue;
          if ((std::abs(track.dcaXY()) > DCAxyCustomCut) || (std::abs(track.dcaZ()) > DCAzCustomCut))
            continue;
        }
      }

      if (isPVContributorCut && !track.isPVContributor()) {
        continue;
      }

      if (makeDCAAfterCutPlots) {
        histos.fill(HIST("tracks/dca/after/hDCAxy"), track.dcaXY());
        histos.fill(HIST("tracks/dca/after/hDCAz"), track.dcaZ());
        histos.fill(HIST("tracks/dca/after/hDCAxyVsDCAz"), track.dcaXY(), track.dcaZ());
        histos.fill(HIST("tracks/dca/after/hDCAxyVsPt"), track.pt(), track.dcaXY());
        histos.fill(HIST("tracks/dca/after/hDCAzVsPt"), track.pt(), track.dcaZ());

        if (enablePr) {
          if (std::abs(track.tpcNSigmaPr()) < nsigmaTPCPr) {
            if (track.sign() > 0) {
              histos.fill(HIST("tracks/proton/dca/after/hDCAxyVsPtProton"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/proton/dca/after/hDCAzVsPtProton"), track.pt(), track.dcaZ());
            } else {
              histos.fill(HIST("tracks/proton/dca/after/hDCAxyVsPtantiProton"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/proton/dca/after/hDCAzVsPtantiProton"), track.pt(), track.dcaZ());
            }
          }
        }
        if (enableDe) {
          if (std::abs(track.tpcNSigmaDe()) < nsigmaTPCDe) {
            if (usenITSLayer && !itsClusterMap.test(nITSLayer))
              continue;
            if (track.sign() > 0) {
              histos.fill(HIST("tracks/deuteron/dca/after/hDCAxyVsPtDeuteron"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/deuteron/dca/after/hDCAzVsPtDeuteron"), track.pt(), track.dcaZ());
            } else {
              histos.fill(HIST("tracks/deuteron/dca/after/hDCAxyVsPtantiDeuteron"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/deuteron/dca/after/hDCAzVsPtantiDeuteron"), track.pt(), track.dcaZ());
            }
          }
        }
        if (enableTr) {
          if (isTriton) {
            if (track.sign() > 0) {
              histos.fill(HIST("tracks/triton/dca/after/hDCAxyVsPtTriton"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/triton/dca/after/hDCAzVsPtTriton"), track.pt(), track.dcaZ());
            } else {
              histos.fill(HIST("tracks/triton/dca/after/hDCAxyVsPtantiTriton"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/triton/dca/after/hDCAzVsPtantiTriton"), track.pt(), track.dcaZ());
            }
          }
        }
        if (enableHe) {
          if (std::abs(track.tpcNSigmaHe()) < nsigmaTPCHe) {
            if (track.sign() > 0) {
              histos.fill(HIST("tracks/helium/dca/after/hDCAxyVsPtHelium"), hePt, track.dcaXY());
              histos.fill(HIST("tracks/helium/dca/after/hDCAzVsPtHelium"), hePt, track.dcaZ());
            } else {
              histos.fill(HIST("tracks/helium/dca/after/hDCAxyVsPtantiHelium"), hePt, track.dcaXY());
              histos.fill(HIST("tracks/helium/dca/after/hDCAzVsPtantiHelium"), hePt, track.dcaZ());
            }
          }
        }
        if (enableAl) {
          if (std::abs(track.tpcNSigmaAl()) < nsigmaTPCAl) {
            if (track.sign() > 0) {
              histos.fill(HIST("tracks/alpha/dca/after/hDCAxyVsPtAlpha"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/alpha/dca/after/hDCAzVsPtAlpha"), track.pt(), track.dcaZ());
            } else {
              histos.fill(HIST("tracks/alpha/dca/after/hDCAxyVsPtantiAlpha"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/alpha/dca/after/hDCAzVsPtantiAlpha"), track.pt(), track.dcaZ());
            }
          }
        }
      }

      // LOG(info)<<"\n collisionId ============>"<<track.collisionId();

      // QA histos fill
      histos.fill(HIST("qa/h1ITSncr"), track.itsNCls());
      histos.fill(HIST("qa/h1TPCnfound"), track.tpcNClsFound());
      histos.fill(HIST("qa/h1TPCncr"), track.tpcNClsCrossedRows());
      histos.fill(HIST("qa/h1rTPC"), track.tpcCrossedRowsOverFindableCls());
      histos.fill(HIST("qa/h1chi2ITS"), track.tpcChi2NCl());
      histos.fill(HIST("qa/h1chi2TPC"), track.itsChi2NCl());

      if (enableDebug) {
        histos.fill(HIST("tracks/h1Eta"), track.eta());
        histos.fill(HIST("tracks/h1VarPhi"), track.phi());
        histos.fill(HIST("tracks/h2EtaVsPhi"), track.eta(), track.phi());

        if (track.sign() > 0) {
          histos.fill(HIST("debug/qa/h2TPCncrVsPtPos"), track.tpcInnerParam(), track.tpcNClsCrossedRows());
          histos.fill(HIST("debug/qa/h2TPCncrVsTPCsignalPos"), track.tpcSignal(), track.tpcNClsCrossedRows());

          if (track.tpcInnerParam() < 0.5f) {
            histos.fill(HIST("debug/qa/h1TPCncrLowPPos"), track.tpcNClsCrossedRows());
          }
          if ((track.tpcInnerParam() >= 0.5f) && (track.tpcInnerParam() < 1.f)) {
            histos.fill(HIST("debug/qa/h1TPCncrMidPPos"), track.tpcNClsCrossedRows());
          }
          if (track.tpcInnerParam() >= 1.f) {
            histos.fill(HIST("debug/qa/h1TPCncrHighPPos"), track.tpcNClsCrossedRows());
          }
        } else {
          histos.fill(HIST("debug/qa/h2TPCncrVsPtNeg"), track.tpcInnerParam(), track.tpcNClsCrossedRows());
          histos.fill(HIST("debug/qa/h2TPCncrVsTPCsignalNeg"), track.tpcSignal(), track.tpcNClsCrossedRows());

          if (track.tpcInnerParam() < 0.5f) {
            histos.fill(HIST("debug/qa/h1TPCncrLowPNeg"), track.tpcNClsCrossedRows());
          }
          if ((track.tpcInnerParam() >= 0.5f) && (track.tpcInnerParam() < 1.f)) {
            histos.fill(HIST("debug/qa/h1TPCncrMidPNeg"), track.tpcNClsCrossedRows());
          }
          if (track.tpcInnerParam() >= 1.f) {
            histos.fill(HIST("debug/qa/h1TPCncrHighPNeg"), track.tpcNClsCrossedRows());
          }
        }

        histos.fill(HIST("debug/h2TPCsignVsTPCmomentumFull"), track.tpcInnerParam() / (1.f * track.sign()), track.tpcSignal());
        histos.fill(HIST("tracks/pion/h2PionVspTNSigmaTPC"), track.pt(), track.tpcNSigmaPi());
        histos.fill(HIST("tracks/kaon/h2KaonVspTNSigmaTPC"), track.pt(), track.tpcNSigmaKa());
      }

      if (enablePtSpectra)
        histos.fill(HIST("tracks/eff/h2pVsTPCmomentum"), track.tpcInnerParam(), track.p());

      if (enableFiltering) {
        if (track.tpcNSigmaKa() < 5)
          continue;
      }

      // p,d,t,He
      histos.fill(HIST("tracks/h2TPCsignVsTPCmomentum"), track.tpcInnerParam() / (1.f * track.sign()), track.tpcSignal());

      if (doTPCcalib)
        return;

      if (track.sign() > 0) {
        if (enablePr) {
          if (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Proton))) < yCut) {
            if (enableExpSignalTPC)
              histos.fill(HIST("tracks/proton/h2ProtonTPCExpSignalDiffVsPt"), track.pt(), track.tpcExpSignalDiffPr());

            switch (useHasTRDConfig) {
              case 0:
                histos.fill(HIST("tracks/proton/h2ProtonVspTNSigmaTPC"), track.pt(), track.tpcNSigmaPr());
                break;
              case 1:
                if (track.hasTRD()) {
                  histos.fill(HIST("tracks/proton/h2ProtonVspTNSigmaTPC"), track.pt(), track.tpcNSigmaPr());
                }
                break;
              case 2:
                if (!track.hasTRD()) {
                  histos.fill(HIST("tracks/proton/h2ProtonVspTNSigmaTPC"), track.pt(), track.tpcNSigmaPr());
                }
                break;
            }
          }
        }
        if (enableDe && deRapCut) {
          if (enableExpSignalTPC)
            histos.fill(HIST("tracks/deuteron/h2DeuteronTPCExpSignalDiffVsPt"), track.pt(), track.tpcExpSignalDiffDe());

          switch (useHasTRDConfig) {
            case 0:
              histos.fill(HIST("tracks/deuteron/h2DeuteronVspTNSigmaTPC"), track.pt(), track.tpcNSigmaDe());
              break;
            case 1:
              if (track.hasTRD()) {
                histos.fill(HIST("tracks/deuteron/h2DeuteronVspTNSigmaTPC"), track.pt(), track.tpcNSigmaDe());
              }
              break;
            case 2:
              if (!track.hasTRD()) {
                histos.fill(HIST("tracks/deuteron/h2DeuteronVspTNSigmaTPC"), track.pt(), track.tpcNSigmaDe());
              }
              break;
          }
        }
        if (enableTr) {
          if (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Triton))) < yCut) {
            histos.fill(HIST("tracks/triton/h2TritonVspTNSigmaTPC"), track.pt(), track.tpcNSigmaTr());
          }
        }
        if (enableHe && heRapCut) {
          if (enableExpSignalTPC)
            histos.fill(HIST("tracks/helium/h2HeliumTPCExpSignalDiffVsPt"), hePt, track.tpcExpSignalDiffHe());

          histos.fill(HIST("tracks/helium/h2HeliumVspTNSigmaTPC"), hePt, track.tpcNSigmaHe());
        }
        if (enableAl) {
          if (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Alpha))) < yCut) {
            histos.fill(HIST("tracks/alpha/h2AlphaVspTNSigmaTPC"), track.pt(), track.tpcNSigmaAl());
          }
        }
      } else {
        if (enablePr) {
          if (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Proton))) < yCut) {
            if (enableExpSignalTPC)
              histos.fill(HIST("tracks/proton/h2antiProtonTPCExpSignalDiffVsPt"), track.pt(), track.tpcExpSignalDiffPr());

            switch (useHasTRDConfig) {
              case 0:
                histos.fill(HIST("tracks/proton/h2antiProtonVspTNSigmaTPC"), track.pt(), track.tpcNSigmaPr());
                break;
              case 1:
                if (track.hasTRD()) {
                  histos.fill(HIST("tracks/proton/h2antiProtonVspTNSigmaTPC"), track.pt(), track.tpcNSigmaPr());
                }
                break;
              case 2:
                if (!track.hasTRD()) {
                  histos.fill(HIST("tracks/proton/h2antiProtonVspTNSigmaTPC"), track.pt(), track.tpcNSigmaPr());
                }
                break;
            }
          }
        }
        if (enableDe && deRapCut) {
          if (enableExpSignalTPC)
            histos.fill(HIST("tracks/deuteron/h2antiDeuteronTPCExpSignalDiffVsPt"), track.pt(), track.tpcExpSignalDiffDe());

          switch (useHasTRDConfig) {
            case 0:
              histos.fill(HIST("tracks/deuteron/h2antiDeuteronVspTNSigmaTPC"), track.pt(), track.tpcNSigmaDe());
              break;
            case 1:
              if (track.hasTRD()) {
                histos.fill(HIST("tracks/deuteron/h2antiDeuteronVspTNSigmaTPC"), track.pt(), track.tpcNSigmaDe());
              }
              break;
            case 2:
              if (!track.hasTRD()) {
                histos.fill(HIST("tracks/deuteron/h2antiDeuteronVspTNSigmaTPC"), track.pt(), track.tpcNSigmaDe());
              }
              break;
          }
        }
        if (enableTr) {
          if (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Triton))) < yCut) {
            histos.fill(HIST("tracks/triton/h2antiTritonVspTNSigmaTPC"), track.pt(), track.tpcNSigmaTr());
          }
        }
        if (enableHe && heRapCut) {
          if (enableExpSignalTPC)
            histos.fill(HIST("tracks/helium/h2antiHeliumTPCExpSignalDiffVsPt"), hePt, track.tpcExpSignalDiffHe());

          histos.fill(HIST("tracks/helium/h2antiHeliumVspTNSigmaTPC"), hePt, track.tpcNSigmaHe());
        }
        if (enableAl) {
          if (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Alpha))) < yCut) {
            histos.fill(HIST("tracks/alpha/h2antiAlphaVspTNSigmaTPC"), track.pt(), track.tpcNSigmaAl());
          }
        }
      }

      //  TOF
      if (doTOFplots) {
        histos.fill(HIST("tracks/pion/h2PionVspTNSigmaTOF"), track.pt(), track.tofNSigmaPi());
        histos.fill(HIST("tracks/kaon/h2KaonVspTNSigmaTOF"), track.pt(), track.tofNSigmaKa());
        if (track.sign() > 0) {
          if (enablePr) {
            if (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Proton))) < yCut) {
              switch (useHasTRDConfig) {
                case 0:
                  histos.fill(HIST("tracks/proton/h2ProtonVspTNSigmaTOF"), track.pt(), track.tofNSigmaPr());
                  break;
                case 1:
                  if (track.hasTRD()) {
                    histos.fill(HIST("tracks/proton/h2ProtonVspTNSigmaTOF"), track.pt(), track.tofNSigmaPr());
                  }
                  break;
                case 2:
                  if (!track.hasTRD()) {
                    histos.fill(HIST("tracks/proton/h2ProtonVspTNSigmaTOF"), track.pt(), track.tofNSigmaPr());
                  }
                  break;
              }
              if (enableExpSignalTOF)
                histos.fill(HIST("tracks/proton/h2ProtonTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffPr());
            }
          }
          if (enableDe && deRapCut) {
            switch (useHasTRDConfig) {
              case 0:
                histos.fill(HIST("tracks/deuteron/h2DeuteronVspTNSigmaTOF"), track.pt(), track.tofNSigmaDe());
                break;
              case 1:
                if (track.hasTRD()) {
                  histos.fill(HIST("tracks/deuteron/h2DeuteronVspTNSigmaTOF"), track.pt(), track.tofNSigmaDe());
                }
                break;
              case 2:
                if (!track.hasTRD()) {
                  histos.fill(HIST("tracks/deuteron/h2DeuteronVspTNSigmaTOF"), track.pt(), track.tofNSigmaDe());
                }
                break;
            }
            if (enableExpSignalTOF)
              histos.fill(HIST("tracks/deuteron/h2DeuteronTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffDe());
          }
          if (enableHe && heRapCut) {
            histos.fill(HIST("tracks/helium/h2HeliumVspTNSigmaTOF"), hePt, track.tofNSigmaHe());
            if (enableExpSignalTOF)
              histos.fill(HIST("tracks/helium/h2HeliumTOFExpSignalDiffVsPt"), hePt, track.tofExpSignalDiffHe());
          }
          if (enableEvTimeSplitting && track.hasTOF()) {
            if (track.isEvTimeTOF() && track.isEvTimeT0AC()) {
              if (enablePr)
                histos.fill(HIST("tracks/evtime/ft0tof/proton/h2ProtonVspTNSigmaTOF"), track.pt(), track.tofNSigmaPr());
              if (enableDe)
                histos.fill(HIST("tracks/evtime/ft0tof/deuteron/h2DeuteronVspTNSigmaTOF"), track.pt(), track.tofNSigmaDe());
              if (enableExpSignalTOF) {
                if (enablePr)
                  histos.fill(HIST("tracks/evtime/ft0tof/proton/h2ProtonTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffPr());
                if (enableDe)
                  histos.fill(HIST("tracks/evtime/ft0tof/deuteron/h2DeuteronTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffDe());
              }
              if (enablePr)
                histos.fill(HIST("tracks/evtime/ft0tof/proton/h3ProtonNSigmaTPCvsNSigmaTOFvsPt"), track.tpcNSigmaPr(), track.tofNSigmaPr(), track.pt());
              if (enableDe)
                histos.fill(HIST("tracks/evtime/ft0tof/deuteron/h3DeuteronNSigmaTPCvsNSigmaTOFvsPt"), track.tpcNSigmaDe(), track.tofNSigmaDe(), track.pt());
              if (enableDebug && (track.beta() > betaCut)) {
                if (enablePr)
                  histos.fill(HIST("debug/evtime/ft0tof/proton/h2ProtonVspTNSigmaTPC_BetaCut"), track.pt(), track.tpcNSigmaPr());
                if (enableDe)
                  histos.fill(HIST("debug/evtime/ft0tof/deuteron/h2DeuteronVspTNSigmaTPC_BetaCut"), track.pt(), track.tpcNSigmaDe());

                if (enablePr)
                  histos.fill(HIST("debug/evtime/ft0tof/proton/h2ProtonVspTNSigmaTOF_BetaCut"), track.pt(), track.tofNSigmaPr());
                if (enableDe)
                  histos.fill(HIST("debug/evtime/ft0tof/deuteron/h2DeuteronVspTNSigmaTOF_BetaCut"), track.pt(), track.tofNSigmaDe());
                if (enableExpSignalTOF) {
                  histos.fill(HIST("debug/evtime/ft0tof/proton/h2ProtonTOFExpSignalDiffVsPt_BetaCut"), track.pt(), track.tofExpSignalDiffPr());
                  if (enableDe)
                    histos.fill(HIST("debug/evtime/ft0tof/deuteron/h2DeuteronTOFExpSignalDiffVsPt_BetaCut"), track.pt(), track.tofExpSignalDiffDe());
                }
              }

            } else if (track.isEvTimeT0AC()) {
              if (enablePr)
                histos.fill(HIST("tracks/evtime/ft0/proton/h2ProtonVspTNSigmaTOF"), track.pt(), track.tofNSigmaPr());
              if (enableDe)
                histos.fill(HIST("tracks/evtime/ft0/deuteron/h2DeuteronVspTNSigmaTOF"), track.pt(), track.tofNSigmaDe());
              if (enableExpSignalTOF) {
                if (enablePr)
                  histos.fill(HIST("tracks/evtime/ft0/proton/h2ProtonTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffPr());
                if (enableDe)
                  histos.fill(HIST("tracks/evtime/ft0/deuteron/h2DeuteronTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffDe());
              }
              if (enablePr)
                histos.fill(HIST("tracks/evtime/ft0/proton/h3ProtonNSigmaTPCvsNSigmaTOFvsPt"), track.tpcNSigmaPr(), track.tofNSigmaPr(), track.pt());
              if (enableDe)
                histos.fill(HIST("tracks/evtime/ft0/deuteron/h3DeuteronNSigmaTPCvsNSigmaTOFvsPt"), track.tpcNSigmaDe(), track.tofNSigmaDe(), track.pt());
              if (enableDebug && (track.beta() > betaCut)) {
                if (enablePr)
                  histos.fill(HIST("debug/evtime/ft0/proton/h2ProtonVspTNSigmaTPC_BetaCut"), track.pt(), track.tpcNSigmaPr());
                if (enableDe)
                  histos.fill(HIST("debug/evtime/ft0/deuteron/h2DeuteronVspTNSigmaTPC_BetaCut"), track.pt(), track.tpcNSigmaDe());

                if (enablePr)
                  histos.fill(HIST("debug/evtime/ft0/proton/h2ProtonVspTNSigmaTOF_BetaCut"), track.pt(), track.tofNSigmaPr());
                if (enableDe)
                  histos.fill(HIST("debug/evtime/ft0/deuteron/h2DeuteronVspTNSigmaTOF_BetaCut"), track.pt(), track.tofNSigmaDe());
                if (enableExpSignalTOF) {
                  if (enablePr)
                    histos.fill(HIST("debug/evtime/ft0/proton/h2ProtonTOFExpSignalDiffVsPt_BetaCut"), track.pt(), track.tofExpSignalDiffPr());
                  if (enableDe)
                    histos.fill(HIST("debug/evtime/ft0/deuteron/h2DeuteronTOFExpSignalDiffVsPt_BetaCut"), track.pt(), track.tofExpSignalDiffDe());
                }
              }

            } else if (track.isEvTimeTOF()) {
              if (enablePr)
                histos.fill(HIST("tracks/evtime/tof/proton/h2ProtonVspTNSigmaTOF"), track.pt(), track.tofNSigmaPr());
              if (enableDe)
                histos.fill(HIST("tracks/evtime/tof/deuteron/h2DeuteronVspTNSigmaTOF"), track.pt(), track.tofNSigmaDe());
              if (enableExpSignalTOF) {
                if (enablePr)
                  histos.fill(HIST("tracks/evtime/tof/proton/h2ProtonTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffPr());
                if (enableDe)
                  histos.fill(HIST("tracks/evtime/tof/deuteron/h2DeuteronTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffDe());
              }
              if (enablePr)
                histos.fill(HIST("tracks/evtime/tof/proton/h3ProtonNSigmaTPCvsNSigmaTOFvsPt"), track.tpcNSigmaPr(), track.tofNSigmaPr(), track.pt());
              if (enableDe)
                histos.fill(HIST("tracks/evtime/tof/deuteron/h3DeuteronNSigmaTPCvsNSigmaTOFvsPt"), track.tpcNSigmaDe(), track.tofNSigmaDe(), track.pt());
              if (enableDebug && (track.beta() > betaCut)) {
                if (enablePr)
                  histos.fill(HIST("debug/evtime/tof/proton/h2ProtonVspTNSigmaTPC_BetaCut"), track.pt(), track.tpcNSigmaPr());
                if (enableDe)
                  histos.fill(HIST("debug/evtime/tof/deuteron/h2DeuteronVspTNSigmaTPC_BetaCut"), track.pt(), track.tpcNSigmaDe());

                if (enablePr)
                  histos.fill(HIST("debug/evtime/tof/proton/h2ProtonVspTNSigmaTOF_BetaCut"), track.pt(), track.tofNSigmaPr());
                if (enableDe)
                  histos.fill(HIST("debug/evtime/tof/deuteron/h2DeuteronVspTNSigmaTOF_BetaCut"), track.pt(), track.tofNSigmaDe());

                if (enableExpSignalTOF) {
                  if (enablePr)
                    histos.fill(HIST("debug/evtime/tof/proton/h2ProtonTOFExpSignalDiffVsPt_BetaCut"), track.pt(), track.tofExpSignalDiffPr());
                  if (enableDe)
                    histos.fill(HIST("debug/evtime/tof/deuteron/h2DeuteronTOFExpSignalDiffVsPt_BetaCut"), track.pt(), track.tofExpSignalDiffDe());
                }
              }

            } else {
              if (enablePr)
                histos.fill(HIST("tracks/evtime/fill/proton/h2ProtonVspTNSigmaTOF"), track.pt(), track.tofNSigmaPr());
              if (enableDe)
                histos.fill(HIST("tracks/evtime/fill/deuteron/h2DeuteronVspTNSigmaTOF"), track.pt(), track.tofNSigmaDe());
              if (enableExpSignalTOF) {
                if (enablePr)
                  histos.fill(HIST("tracks/evtime/fill/proton/h2ProtonTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffPr());
                if (enableDe)
                  histos.fill(HIST("tracks/evtime/fill/deuteron/h2DeuteronTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffDe());
              }
              if (enablePr)
                histos.fill(HIST("tracks/evtime/fill/proton/h3ProtonNSigmaTPCvsNSigmaTOFvsPt"), track.tpcNSigmaPr(), track.tofNSigmaPr(), track.pt());
              if (enableDe)
                histos.fill(HIST("tracks/evtime/fill/deuteron/h3DeuteronNSigmaTPCvsNSigmaTOFvsPt"), track.tpcNSigmaDe(), track.tofNSigmaDe(), track.pt());
              if (enableDebug && (track.beta() > betaCut)) {
                if (enablePr)
                  histos.fill(HIST("debug/evtime/fill/proton/h2ProtonVspTNSigmaTPC_BetaCut"), track.pt(), track.tpcNSigmaPr());
                if (enableDe)
                  histos.fill(HIST("debug/evtime/fill/deuteron/h2DeuteronVspTNSigmaTPC_BetaCut"), track.pt(), track.tpcNSigmaDe());

                if (enablePr)
                  histos.fill(HIST("debug/evtime/fill/proton/h2ProtonVspTNSigmaTOF_BetaCut"), track.pt(), track.tofNSigmaPr());
                if (enableDe)
                  histos.fill(HIST("debug/evtime/fill/deuteron/h2DeuteronVspTNSigmaTOF_BetaCut"), track.pt(), track.tofNSigmaDe());

                if (enableExpSignalTOF) {
                  if (enablePr)
                    histos.fill(HIST("debug/evtime/fill/proton/h2ProtonTOFExpSignalDiffVsPt_BetaCut"), track.pt(), track.tofExpSignalDiffPr());
                  if (enableDe)
                    histos.fill(HIST("debug/evtime/fill/deuteron/h2DeuteronTOFExpSignalDiffVsPt_BetaCut"), track.pt(), track.tofExpSignalDiffDe());
                }
              }
            }
          }

        } else {
          if (enablePr) {
            if (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Proton))) < yCut) {
              switch (useHasTRDConfig) {
                case 0:
                  histos.fill(HIST("tracks/proton/h2antiProtonVspTNSigmaTOF"), track.pt(), track.tofNSigmaPr());
                  break;
                case 1:
                  if (track.hasTRD()) {
                    histos.fill(HIST("tracks/proton/h2antiProtonVspTNSigmaTOF"), track.pt(), track.tofNSigmaPr());
                  }
                  break;
                case 2:
                  if (!track.hasTRD()) {
                    histos.fill(HIST("tracks/proton/h2antiProtonVspTNSigmaTOF"), track.pt(), track.tofNSigmaPr());
                  }
                  break;
              }
              if (enableExpSignalTOF)
                histos.fill(HIST("tracks/proton/h2antiProtonTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffPr());
            }
          }
          if (enableDe && deRapCut) {
            switch (useHasTRDConfig) {
              case 0:
                histos.fill(HIST("tracks/deuteron/h2antiDeuteronVspTNSigmaTOF"), track.pt(), track.tofNSigmaDe());
                break;
              case 1:
                if (track.hasTRD()) {
                  histos.fill(HIST("tracks/deuteron/h2antiDeuteronVspTNSigmaTOF"), track.pt(), track.tofNSigmaDe());
                }
                break;
              case 2:
                if (!track.hasTRD()) {
                  histos.fill(HIST("tracks/deuteron/h2antiDeuteronVspTNSigmaTOF"), track.pt(), track.tofNSigmaDe());
                }
                break;
            }
            if (enableExpSignalTOF)
              histos.fill(HIST("tracks/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffDe());
          }
          if (enableHe && heRapCut) {
            histos.fill(HIST("tracks/helium/h2antiHeliumVspTNSigmaTOF"), hePt, track.tofNSigmaHe());
            if (enableExpSignalTOF)
              histos.fill(HIST("tracks/helium/h2antiHeliumTOFExpSignalDiffVsPt"), hePt, track.tofExpSignalDiffHe());
          }
          if (enableEvTimeSplitting && track.hasTOF()) {
            if (track.isEvTimeTOF() && track.isEvTimeT0AC()) {
              if (enablePr)
                histos.fill(HIST("tracks/evtime/ft0tof/proton/h2antiProtonVspTNSigmaTOF"), track.pt(), track.tofNSigmaPr());
              if (enableDe)
                histos.fill(HIST("tracks/evtime/ft0tof/deuteron/h2antiDeuteronVspTNSigmaTOF"), track.pt(), track.tofNSigmaDe());
              if (enableExpSignalTOF) {
                if (enablePr)
                  histos.fill(HIST("tracks/evtime/ft0tof/proton/h2antiProtonTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffPr());
                if (enableDe)
                  histos.fill(HIST("tracks/evtime/ft0tof/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffDe());
              }
              if (enablePr)
                histos.fill(HIST("tracks/evtime/ft0tof/proton/h3antiProtonNSigmaTPCvsNSigmaTOFvsPt"), track.tpcNSigmaPr(), track.tofNSigmaPr(), track.pt());
              if (enableDe)
                histos.fill(HIST("tracks/evtime/ft0tof/deuteron/h3antiDeuteronNSigmaTPCvsNSigmaTOFvsPt"), track.tpcNSigmaDe(), track.tofNSigmaDe(), track.pt());
              if (enableDebug && (track.beta() > betaCut)) {
                if (enablePr)
                  histos.fill(HIST("debug/evtime/ft0tof/proton/h2antiProtonVspTNSigmaTPC_BetaCut"), track.pt(), track.tpcNSigmaPr());
                if (enableDe)
                  histos.fill(HIST("debug/evtime/ft0tof/deuteron/h2antiDeuteronVspTNSigmaTPC_BetaCut"), track.pt(), track.tpcNSigmaDe());
                if (enablePr)
                  histos.fill(HIST("debug/evtime/ft0tof/proton/h2antiProtonVspTNSigmaTOF_BetaCut"), track.pt(), track.tofNSigmaPr());
                if (enableDe)
                  histos.fill(HIST("debug/evtime/ft0tof/deuteron/h2antiDeuteronVspTNSigmaTOF_BetaCut"), track.pt(), track.tofNSigmaDe());

                if (enableExpSignalTOF) {
                  if (enablePr)
                    histos.fill(HIST("debug/evtime/ft0tof/proton/h2antiProtonTOFExpSignalDiffVsPt_BetaCut"), track.pt(), track.tofExpSignalDiffPr());
                  if (enableDe)
                    histos.fill(HIST("debug/evtime/ft0tof/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt_BetaCut"), track.pt(), track.tofExpSignalDiffDe());
                }
              }

            } else if (track.isEvTimeT0AC()) {
              if (enablePr)
                histos.fill(HIST("tracks/evtime/ft0/proton/h2antiProtonVspTNSigmaTOF"), track.pt(), track.tofNSigmaPr());
              if (enableDe)
                histos.fill(HIST("tracks/evtime/ft0/deuteron/h2antiDeuteronVspTNSigmaTOF"), track.pt(), track.tofNSigmaDe());
              if (enableExpSignalTOF) {
                if (enablePr)
                  histos.fill(HIST("tracks/evtime/ft0/proton/h2antiProtonTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffPr());
                if (enableDe)
                  histos.fill(HIST("tracks/evtime/ft0/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffDe());
                if (enablePr)
                  histos.fill(HIST("tracks/evtime/ft0/proton/h3antiProtonNSigmaTPCvsNSigmaTOFvsPt"), track.tpcNSigmaPr(), track.tofNSigmaPr(), track.pt());
                if (enablePr)
                  histos.fill(HIST("debug/evtime/ft0/proton/h2antiProtonVspTNSigmaTPC_BetaCut"), track.pt(), track.tpcNSigmaPr());
                if (enableDe)
                  histos.fill(HIST("debug/evtime/ft0/deuteron/h2antiDeuteronVspTNSigmaTPC_BetaCut"), track.pt(), track.tpcNSigmaDe());
                if (enablePr)
                  histos.fill(HIST("debug/evtime/ft0/proton/h2antiProtonVspTNSigmaTOF_BetaCut"), track.pt(), track.tofNSigmaPr());
                if (enableDe)
                  histos.fill(HIST("debug/evtime/ft0/deuteron/h2antiDeuteronVspTNSigmaTOF_BetaCut"), track.pt(), track.tofNSigmaDe());

                if (enableExpSignalTOF) {
                  if (enablePr)
                    histos.fill(HIST("debug/evtime/ft0/proton/h2antiProtonTOFExpSignalDiffVsPt_BetaCut"), track.pt(), track.tofExpSignalDiffPr());
                  if (enableDe)
                    histos.fill(HIST("debug/evtime/ft0/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt_BetaCut"), track.pt(), track.tofExpSignalDiffDe());
                }
              }

            } else if (track.isEvTimeTOF()) {
              if (enablePr)
                histos.fill(HIST("tracks/evtime/tof/proton/h2antiProtonVspTNSigmaTOF"), track.pt(), track.tofNSigmaPr());
              if (enableDe)
                histos.fill(HIST("tracks/evtime/tof/deuteron/h2antiDeuteronVspTNSigmaTOF"), track.pt(), track.tofNSigmaDe());
              if (enableExpSignalTOF) {
                if (enablePr)
                  histos.fill(HIST("tracks/evtime/tof/proton/h2antiProtonTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffPr());
                if (enableDe)
                  histos.fill(HIST("tracks/evtime/tof/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffDe());
                if (enablePr)
                  histos.fill(HIST("tracks/evtime/tof/proton/h3antiProtonNSigmaTPCvsNSigmaTOFvsPt"), track.tpcNSigmaPr(), track.tofNSigmaPr(), track.pt());
                if (enablePr)
                  histos.fill(HIST("debug/evtime/tof/proton/h2antiProtonVspTNSigmaTPC_BetaCut"), track.pt(), track.tpcNSigmaPr());
                if (enableDe)
                  histos.fill(HIST("debug/evtime/tof/deuteron/h2antiDeuteronVspTNSigmaTPC_BetaCut"), track.pt(), track.tpcNSigmaDe());

                if (enablePr)
                  histos.fill(HIST("debug/evtime/tof/proton/h2antiProtonVspTNSigmaTOF_BetaCut"), track.pt(), track.tofNSigmaPr());
                if (enableDe)
                  histos.fill(HIST("debug/evtime/tof/deuteron/h2antiDeuteronVspTNSigmaTOF_BetaCut"), track.pt(), track.tofNSigmaDe());

                if (enableExpSignalTOF) {
                  if (enablePr)
                    histos.fill(HIST("debug/evtime/tof/proton/h2antiProtonTOFExpSignalDiffVsPt_BetaCut"), track.pt(), track.tofExpSignalDiffPr());
                  if (enableDe)
                    histos.fill(HIST("debug/evtime/tof/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt_BetaCut"), track.pt(), track.tofExpSignalDiffDe());
                }
              }

            } else {
              if (enablePr)
                histos.fill(HIST("tracks/evtime/fill/proton/h2antiProtonVspTNSigmaTOF"), track.pt(), track.tofNSigmaPr());
              if (enableDe)
                histos.fill(HIST("tracks/evtime/fill/deuteron/h2antiDeuteronVspTNSigmaTOF"), track.pt(), track.tofNSigmaDe());
              if (enableExpSignalTOF) {
                if (enablePr)
                  histos.fill(HIST("tracks/evtime/fill/proton/h2antiProtonTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffPr());
                if (enableDe)
                  histos.fill(HIST("tracks/evtime/fill/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt"), track.pt(), track.tofExpSignalDiffDe());
                if (enablePr)
                  histos.fill(HIST("tracks/evtime/fill/proton/h3antiProtonNSigmaTPCvsNSigmaTOFvsPt"), track.tpcNSigmaPr(), track.tofNSigmaPr(), track.pt());
                if (enablePr)
                  histos.fill(HIST("debug/evtime/fill/proton/h2antiProtonVspTNSigmaTPC_BetaCut"), track.pt(), track.tpcNSigmaPr());
                if (enableDe)
                  histos.fill(HIST("debug/evtime/fill/deuteron/h2antiDeuteronVspTNSigmaTPC_BetaCut"), track.pt(), track.tpcNSigmaDe());

                if (enablePr)
                  histos.fill(HIST("debug/evtime/fill/proton/h2antiProtonVspTNSigmaTOF_BetaCut"), track.pt(), track.tofNSigmaPr());
                if (enableDe)
                  histos.fill(HIST("debug/evtime/fill/deuteron/h2antiDeuteronVspTNSigmaTOF_BetaCut"), track.pt(), track.tofNSigmaDe());

                if (enableExpSignalTOF) {
                  if (enablePr)
                    histos.fill(HIST("debug/evtime/fill/proton/h2antiProtonTOFExpSignalDiffVsPt_BetaCut"), track.pt(), track.tofExpSignalDiffPr());
                  if (enableDe)
                    histos.fill(HIST("debug/evtime/fill/deuteron/h2antiDeuteronTOFExpSignalDiffVsPt_BetaCut"), track.pt(), track.tofExpSignalDiffDe());
                }
              }
            }
          }
        }
      }

      // PID
      if (enablePr) {
        if ((std::abs(track.tpcNSigmaPr()) < nsigmaTPCPr) && (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Proton))) < yCut)) {
          if (track.sign() > 0) {
            if (enablePtSpectra) {
              histos.fill(HIST("tracks/eff/proton/hPtPr"), track.pt());
              histos.fill(HIST("tracks/eff/proton/h2pVsTPCmomentumPr"), track.tpcInnerParam(), track.p());
            }
            histos.fill(HIST("tracks/proton/h1ProtonSpectra"), track.pt());
            if (enablePIDplot)
              histos.fill(HIST("tracks/proton/h2TPCsignVsTPCmomentumProton"), track.tpcInnerParam(), track.tpcSignal());
            if (enableNucleiHardCut && (std::abs(track.tpcNSigmaPi()) > 2) && (std::abs(track.tpcNSigmaKa()) > 2) && (std::abs(track.tpcNSigmaDe()) > 2)) {
              histos.fill(HIST("tracks/proton/hc/h2TPCsignVsTPCmomentumProton_hard"), track.tpcInnerParam(), track.tpcSignal());
            }
          } else {
            if (enablePtSpectra) {
              histos.fill(HIST("tracks/eff/proton/hPtantiPr"), track.pt());
              histos.fill(HIST("tracks/eff/proton/h2pVsTPCmomentumantiPr"), track.tpcInnerParam(), track.p());
            }
            histos.fill(HIST("tracks/proton/h1antiProtonSpectra"), track.pt());
            if (enablePIDplot)
              histos.fill(HIST("tracks/proton/h2TPCsignVsTPCmomentumantiProton"), track.tpcInnerParam(), track.tpcSignal());
            if (enableNucleiHardCut && (std::abs(track.tpcNSigmaPi()) > 2) && (std::abs(track.tpcNSigmaKa()) > 2) && (std::abs(track.tpcNSigmaDe()) > 2)) {
              histos.fill(HIST("tracks/proton/hc/h2TPCsignVsTPCmomentumantiProton_hard"), track.tpcInnerParam(), track.tpcSignal());
            }
          }
        }
      }

      if (enableDe) {
        // if ((((!enableStrongCut) && (std::abs(track.tpcNSigmaDe()) < nsigmaTPCDe)) || ((enableStrongCut) && (std::abs(track.tpcNSigmaPr()) >= nsigmaTPCStrongCut))) && (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Deuteron))) < yCut)) {
        if ((std::abs(track.tpcNSigmaDe()) < nsigmaTPCDe) && deRapCut) {
          if (track.sign() > 0) {
            if (enablePtSpectra) {
              histos.fill(HIST("tracks/eff/deuteron/hPtDe"), track.pt());
              histos.fill(HIST("tracks/eff/deuteron/h2pVsTPCmomentumDe"), track.tpcInnerParam(), track.p());
            }
            histos.fill(HIST("tracks/deuteron/h1DeuteronSpectra"), track.pt());
            if (enablePIDplot)
              histos.fill(HIST("tracks/deuteron/h2TPCsignVsTPCmomentumDeuteron"), track.tpcInnerParam(), track.tpcSignal());
            if (enableNucleiHardCut && (std::abs(track.tpcNSigmaPi()) > 2) && (std::abs(track.tpcNSigmaKa()) > 2) && (std::abs(track.tpcNSigmaPr()) > 1) && (std::abs(track.tpcNSigmaTr()) > 1) && (std::abs(track.tpcNSigmaDe()) < nsigmaTPCStrongCut)) {
              histos.fill(HIST("tracks/deuteron/hc/h2TPCsignVsTPCmomentumDeuteron_hard"), track.tpcInnerParam(), track.tpcSignal());
            }
          } else {
            if (enablePtSpectra) {
              histos.fill(HIST("tracks/eff/deuteron/hPtantiDe"), track.pt());
              histos.fill(HIST("tracks/eff/deuteron/h2pVsTPCmomentumantiDe"), track.tpcInnerParam(), track.p());
            }
            histos.fill(HIST("tracks/deuteron/h1antiDeuteronSpectra"), track.pt());
            if (enablePIDplot)
              histos.fill(HIST("tracks/deuteron/h2TPCsignVsTPCmomentumantiDeuteron"), track.tpcInnerParam(), track.tpcSignal());
            if (enableNucleiHardCut && (std::abs(track.tpcNSigmaPi()) > 2) && (std::abs(track.tpcNSigmaKa()) > 2) && (std::abs(track.tpcNSigmaPr()) > 1) && (std::abs(track.tpcNSigmaTr()) > 1) && (std::abs(track.tpcNSigmaDe()) < nsigmaTPCStrongCut)) {
              histos.fill(HIST("tracks/deuteron/hc/h2TPCsignVsTPCmomentumantiDeuteron_hard"), track.tpcInnerParam(), track.tpcSignal());
            }
          }
        }
      }

      if (enableTr) {
        // if ((((!enableStrongCut) && (std::abs(track.tpcNSigmaTr()) < nsigmaTPCTr)) || ((enableStrongCut) && (std::abs(track.tpcNSigmaPr()) >= nsigmaTPCStrongCut))) && (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Triton))) < yCut)) {
        if ((isTriton) && (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Triton))) < yCut)) {
          if (track.sign() > 0) {
            if (enablePtSpectra) {
              histos.fill(HIST("tracks/eff/triton/hPtTr"), track.pt());
              histos.fill(HIST("tracks/eff/triton/h2pVsTPCmomentumTr"), track.tpcInnerParam(), track.p());
            }
            histos.fill(HIST("tracks/triton/h1TritonSpectra"), track.pt());
            if (enablePIDplot)
              histos.fill(HIST("tracks/triton/h2TPCsignVsTPCmomentumTriton"), track.tpcInnerParam(), track.tpcSignal());
            if (enableNucleiHardCut && (std::abs(track.tpcNSigmaKa()) > 2) && (std::abs(track.tpcNSigmaPr()) > 2) && (std::abs(track.tpcNSigmaDe()) > 1) && (std::abs(track.tpcNSigmaHe()) > 1) && (std::abs(track.tpcNSigmaTr()) < nsigmaTPCStrongCut)) {
              histos.fill(HIST("tracks/triton/hc/h2TPCsignVsTPCmomentumTriton_hard"), track.tpcInnerParam(), track.tpcSignal());
            }
          } else {
            if (enablePtSpectra) {
              histos.fill(HIST("tracks/eff/triton/hPtantiTr"), track.pt());
              histos.fill(HIST("tracks/eff/triton/h2pVsTPCmomentumantiTr"), track.tpcInnerParam(), track.p());
            }
            histos.fill(HIST("tracks/triton/h1antiTritonSpectra"), track.pt());
            if (enablePIDplot)
              histos.fill(HIST("tracks/triton/h2TPCsignVsTPCmomentumantiTriton"), track.tpcInnerParam(), track.tpcSignal());
            if (enableNucleiHardCut && (std::abs(track.tpcNSigmaKa()) > 2) && (std::abs(track.tpcNSigmaPr()) > 2) && (std::abs(track.tpcNSigmaDe()) > 1) && (std::abs(track.tpcNSigmaHe()) > 1) && (std::abs(track.tpcNSigmaTr()) < nsigmaTPCStrongCut)) {
              histos.fill(HIST("tracks/triton/hc/h2TPCsignVsTPCmomentumantiTriton_hard"), track.tpcInnerParam(), track.tpcSignal());
            }
          }
        }
      }
      if (enableHe) {
        // if ((((!enableStrongCut) && (std::abs(track.tpcNSigmaHe()) < nsigmaTPCHe)) || ((enableStrongCut) && (std::abs(track.tpcNSigmaPr()) >= nsigmaTPCStrongCut))) && (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Helium3))) < yCut)) {
        if ((std::abs(track.tpcNSigmaHe()) < nsigmaTPCHe) && heRapCut) {
          if (track.sign() > 0) {
            if (enablePtSpectra) {
              histos.fill(HIST("tracks/eff/helium/hPtHe"), hePt);
              histos.fill(HIST("tracks/eff/helium/h2pVsTPCmomentumHe"), track.tpcInnerParam(), track.p());
            }
            histos.fill(HIST("tracks/helium/h1HeliumSpectra"), hePt);
            if (enablePIDplot)
              histos.fill(HIST("tracks/helium/h2TPCsignVsTPCmomentumHelium"), track.tpcInnerParam(), track.tpcSignal());
            if (enableNucleiHardCut && (std::abs(track.tpcNSigmaKa()) > 2) && (std::abs(track.tpcNSigmaPr()) > 2) && (std::abs(track.tpcNSigmaTr()) > 1) && (std::abs(track.tpcNSigmaHe()) < nsigmaTPCStrongCut)) {
              histos.fill(HIST("tracks/helium/hc/h2TPCsignVsTPCmomentumHelium_hard"), track.tpcInnerParam(), track.tpcSignal());
            }
          } else {
            if (enablePtSpectra) {
              histos.fill(HIST("tracks/eff/helium/hPtantiHe"), hePt);
              histos.fill(HIST("tracks/eff/helium/h2pVsTPCmomentumantiHe"), track.tpcInnerParam(), track.p());
            }
            histos.fill(HIST("tracks/helium/h1antiHeliumSpectra"), hePt);
            if (enablePIDplot)
              histos.fill(HIST("tracks/helium/h2TPCsignVsTPCmomentumantiHelium"), track.tpcInnerParam(), track.tpcSignal());
            if (enableNucleiHardCut && (std::abs(track.tpcNSigmaKa()) > 2) && (std::abs(track.tpcNSigmaPr()) > 2) && (std::abs(track.tpcNSigmaTr()) > 1) && (std::abs(track.tpcNSigmaHe()) < nsigmaTPCStrongCut)) {
              histos.fill(HIST("tracks/helium/hc/h2TPCsignVsTPCmomentumantiHelium_hard"), track.tpcInnerParam(), track.tpcSignal());
            }
          }
        }
      }
      if (enableAl) {
        // if ((((!enableStrongCut) && (std::abs(track.tpcNSigmaAl()) < nsigmaTPCAl)) || ((enableStrongCut) && (std::abs(track.tpcNSigmaPr()) >= nsigmaTPCStrongCut))) && TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Alpha))) < yCut) {
        if ((std::abs(track.tpcNSigmaAl()) < nsigmaTPCAl) && TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Alpha))) < yCut) {
          if (track.sign() > 0) {
            histos.fill(HIST("tracks/alpha/h1AlphaSpectra"), track.pt());
            if (enablePIDplot)
              histos.fill(HIST("tracks/alpha/h2TPCsignVsTPCmomentumAlpha"), track.tpcInnerParam(), track.tpcSignal());
          } else {
            histos.fill(HIST("tracks/alpha/h1antiAlphaSpectra"), track.pt());
            if (enablePIDplot)
              histos.fill(HIST("tracks/alpha/h2TPCsignVsTPCmomentumantiAlpha"), track.tpcInnerParam(), track.tpcSignal());
          }
        }
      }

      if (doTOFplots) {
        if (track.hasTOF()) {
          histos.fill(HIST("tracks/h2TOFbetaVsP_debug"), track.p() / (1.f * track.sign()), track.beta());
          if (enableBetaCut && (track.beta() > betaCut))
            histos.fill(HIST("tracks/h2TOFbetaVsP_BetaCut"), track.p() / (1.f * track.sign()), track.beta());
          switch (useHasTRDConfig) {
            case 0:
              histos.fill(HIST("tracks/h2TOFbetaVsP"), track.p() / (1.f * track.sign()), track.beta());
              break;
            case 1:
              if (track.hasTRD()) {
                histos.fill(HIST("tracks/h2TOFbetaVsP"), track.p() / (1.f * track.sign()), track.beta());
              }
              break;
            case 2:
              if (!track.hasTRD()) {
                histos.fill(HIST("tracks/h2TOFbetaVsP"), track.p() / (1.f * track.sign()), track.beta());
              }
              break;
          }
          if (track.tpcNSigmaPr() + 3 > ProtonNsigmaTPCcutInTOFbeta) {
            histos.fill(HIST("tracks/h2withTPCProtonCutTOFbetaVsP"), track.p() / (1.f * track.sign()), track.beta());
          }

          if (enablePtSpectra)
            histos.fill(HIST("tracks/eff/h2TPCmomentumVsTOFExpMomentum"), track.tofExpMom(), track.tpcInnerParam());

          if (enablePr) {
            if (std::abs(track.tpcNSigmaPr()) < nsigmaTPCPr && track.sign() > 0) {
              histos.fill(HIST("tracks/proton/h2ProtonTOFbetaVsP"), track.p(), track.beta());
              if (enablePtSpectra) {
                histos.fill(HIST("tracks/eff/proton/h2pVsTOFExpMomentumPr"), track.tofExpMom(), track.p());
                histos.fill(HIST("tracks/eff/proton/h2TPCmomentumVsTOFExpMomentumPr"), track.tofExpMom(), track.tpcInnerParam());
              }
            }
            if (std::abs(track.tpcNSigmaPr()) < nsigmaTPCPr && track.sign() < 0) {
              histos.fill(HIST("tracks/proton/h2antiProtonTOFbetaVsP"), track.p(), track.beta());
              if (enablePtSpectra) {
                histos.fill(HIST("tracks/eff/proton/h2pVsTOFExpMomentumantiPr"), track.tofExpMom(), track.p());
                histos.fill(HIST("tracks/eff/proton/h2TPCmomentumVsTOFExpMomentumantiPr"), track.tofExpMom(), track.tpcInnerParam());
              }
            }
          }
          if (enableDe) {
            if (std::abs(track.tpcNSigmaDe()) < nsigmaTPCDe && track.sign() > 0) {
              histos.fill(HIST("tracks/deuteron/h2DeuteronTOFbetaVsP"), track.p(), track.beta());
              if (enablePtSpectra) {
                histos.fill(HIST("tracks/eff/deuteron/h2pVsTOFExpMomentumDe"), track.tofExpMom(), track.p());
                histos.fill(HIST("tracks/eff/deuteron/h2TPCmomentumVsTOFExpMomentumDe"), track.tofExpMom(), track.tpcInnerParam());
              }
            }
            if (std::abs(track.tpcNSigmaDe()) < nsigmaTPCDe && track.sign() < 0) {
              histos.fill(HIST("tracks/deuteron/h2antiDeuteronTOFbetaVsP"), track.p(), track.beta());
              if (enablePtSpectra) {
                histos.fill(HIST("tracks/eff/deuteron/h2pVsTOFExpMomentumantiDe"), track.tofExpMom(), track.p());
                histos.fill(HIST("tracks/eff/deuteron/h2TPCmomentumVsTOFExpMomentumantiDe"), track.tofExpMom(), track.tpcInnerParam());
              }
            }
          }
          if (enableTr) {
            if (isTriton && track.sign() > 0) {
              histos.fill(HIST("tracks/triton/h2TritonTOFbetaVsP"), track.p(), track.beta());
              if (enablePtSpectra) {
                histos.fill(HIST("tracks/eff/triton/h2pVsTOFExpMomentumTr"), track.tofExpMom(), track.p());
                histos.fill(HIST("tracks/eff/triton/h2TPCmomentumVsTOFExpMomentumTr"), track.tofExpMom(), track.tpcInnerParam());
              }
            }
            if (isTriton && track.sign() < 0) {
              histos.fill(HIST("tracks/triton/h2antiTritonTOFbetaVsP"), track.p(), track.beta());
              if (enablePtSpectra) {
                histos.fill(HIST("tracks/eff/triton/h2pVsTOFExpMomentumantiTr"), track.tofExpMom(), track.p());
                histos.fill(HIST("tracks/eff/triton/h2TPCmomentumVsTOFExpMomentumantiTr"), track.tofExpMom(), track.tpcInnerParam());
              }
            }
          }
          if (enableHe) {
            if (std::abs(track.tpcNSigmaHe()) < nsigmaTPCHe && track.sign() > 0) {
              histos.fill(HIST("tracks/helium/h2HeliumTOFbetaVsP"), track.p(), track.beta());
              if (enablePtSpectra) {
                histos.fill(HIST("tracks/eff/helium/h2pVsTOFExpMomentumHe"), track.tofExpMom(), track.p());
                histos.fill(HIST("tracks/eff/helium/h2TPCmomentumVsTOFExpMomentumHe"), track.tofExpMom(), track.tpcInnerParam());
              }
            }
            if (std::abs(track.tpcNSigmaHe()) < nsigmaTPCHe && track.sign() < 0) {
              histos.fill(HIST("tracks/helium/h2antiHeliumTOFbetaVsP"), track.p(), track.beta());
              if (enablePtSpectra) {
                histos.fill(HIST("tracks/eff/helium/h2pVsTOFExpMomentumantiHe"), track.tofExpMom(), track.p());
                histos.fill(HIST("tracks/eff/helium/h2TPCmomentumVsTOFExpMomentumantiHe"), track.tofExpMom(), track.tpcInnerParam());
              }
            }
          }

          if (enableDebug) {
            if (enablePr) {
              if (track.sign() > 0)
                histos.fill(HIST("tracks/deuteron/h3DeuteronNSigmaTPCvsNSigmaTOFvsPt"), track.tpcNSigmaDe(), track.tofNSigmaDe(), track.pt());
              else
                histos.fill(HIST("tracks/proton/h3antiProtonNSigmaTPCvsNSigmaTOFvsPt"), track.tpcNSigmaPr(), track.tofNSigmaPr(), track.pt());
            }
            if (enableDe) {
              if (track.sign() > 0)
                histos.fill(HIST("tracks/deuteron/h3DeuteronNSigmaTPCvsNSigmaTOFvsPt"), track.tpcNSigmaDe(), track.tofNSigmaDe(), track.pt());
              else
                histos.fill(HIST("tracks/deuteron/h3antiDeuteronNSigmaTPCvsNSigmaTOFvsPt"), track.tpcNSigmaDe(), track.tofNSigmaDe(), track.pt());
            }

            if (enableEvTimeSplitting) {
              if (track.isEvTimeTOF() && track.isEvTimeT0AC()) {
                histos.fill(HIST("tracks/evtime/ft0tof/h2TOFbetaVsP"), track.p() / (1.f * track.sign()), track.beta());
                histos.fill(HIST("tracks/evtime/ft0tof/h2TPCsignVsTPCmomentum"), track.tpcInnerParam() / (1.f * track.sign()), track.tpcSignal());
              } else if (track.isEvTimeT0AC()) {
                histos.fill(HIST("tracks/evtime/ft0/h2TOFbetaVsP"), track.p() / (1.f * track.sign()), track.beta());
                histos.fill(HIST("tracks/evtime/ft0/h2TPCsignVsTPCmomentum"), track.tpcInnerParam() / (1.f * track.sign()), track.tpcSignal());
              } else if (track.isEvTimeTOF()) {
                histos.fill(HIST("tracks/evtime/tof/h2TOFbetaVsP"), track.p() / (1.f * track.sign()), track.beta());
                histos.fill(HIST("tracks/evtime/tof/h2TPCsignVsTPCmomentum"), track.tpcInnerParam() / (1.f * track.sign()), track.tpcSignal());
              } else {
                histos.fill(HIST("tracks/evtime/fill/h2TOFbetaVsP"), track.p() / (1.f * track.sign()), track.beta());
                histos.fill(HIST("tracks/evtime/fill/h2TPCsignVsTPCmomentum"), track.tpcInnerParam() / (1.f * track.sign()), track.tpcSignal());
              }
            }
          }

          if ((track.beta() * track.beta()) < 1.) {
            gamma = 1.f / TMath::Sqrt(1.f - (track.beta() * track.beta()));

            switch (massTOFConfig) {
              case 0:
                massTOF = track.tpcInnerParam() * TMath::Sqrt(1.f / (track.beta() * track.beta()) - 1.f);
                break;
              case 1:
                massTOF = track.tofExpMom() * TMath::Sqrt(1.f / (track.beta() * track.beta()) - 1.f);
                break;
            }
            histos.fill(HIST("tracks/h2TPCsignVsBetaGamma"), (track.beta() * gamma) / (1.f * track.sign()), track.tpcSignal());
          } else {
            massTOF = -99.f;
          }

          histos.fill(HIST("tracks/h2TOFmassVsPt"), massTOF, track.pt());

          if (enableEvTimeSplitting) {
            if (track.isEvTimeTOF() && track.isEvTimeT0AC()) {
              histos.fill(HIST("tracks/evtime/ft0tof/h2TOFmassVsPt"), massTOF, track.pt());
            } else if (track.isEvTimeT0AC()) {
              histos.fill(HIST("tracks/evtime/ft0/h2TOFmassVsPt"), massTOF, track.pt());
            } else if (track.isEvTimeTOF()) {
              histos.fill(HIST("tracks/evtime/tof/h2TOFmassVsPt"), massTOF, track.pt());
            } else {
              histos.fill(HIST("tracks/evtime/fill/h2TOFmassVsPt"), massTOF, track.pt());
            }
          }

          if (enablePr) {
            if ((std::abs(track.tpcNSigmaPr()) < nsigmaTPCPr) && (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Proton))) < yCut)) {
              if (enablePtSpectra)
                histos.fill(HIST("tracks/eff/proton/hPtPrTOF"), track.pt());
              if (track.sign() > 0) {
                histos.fill(HIST("tracks/proton/h2TOFmassProtonVsPt"), massTOF, track.pt());
                histos.fill(HIST("tracks/proton/h2TOFmass2ProtonVsPt"), massTOF * massTOF - fMassProton * fMassProton, track.pt());
                if (enableBetaCut && (track.beta() > betaCut)) {
                  histos.fill(HIST("tracks/proton/h2TOFmassProtonVsPt_BetaCut"), massTOF, track.pt());
                  histos.fill(HIST("tracks/proton/h2TOFmass2ProtonVsPt_BetaCut"), massTOF * massTOF - fMassProton * fMassProton, track.pt());
                }
                if (enableExpSignalTOF)
                  histos.fill(HIST("tracks/proton/h2ProtonTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffPr());
                if (enableNucleiHardCut && (std::abs(track.tpcNSigmaKa()) > 2) && (std::abs(track.tpcNSigmaDe()) > 2)) {
                  histos.fill(HIST("tracks/proton/hc/h2TOFmass2ProtonVsPt_hard"), massTOF * massTOF - fMassProton * fMassProton, track.pt());
                }
                if (enableEvTimeSplitting) {
                  if (track.isEvTimeTOF() && track.isEvTimeT0AC()) {
                    histos.fill(HIST("tracks/evtime/ft0tof/proton/h2TOFmass2ProtonVsPt"), massTOF * massTOF - fMassProton * fMassProton, track.pt());
                    // histos.fill(HIST("tracks/evtime/ft0tof/proton/h2ProtonTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffPr());
                  } else if (track.isEvTimeT0AC()) {
                    histos.fill(HIST("tracks/evtime/ft0/proton/h2TOFmass2ProtonVsPt"), massTOF * massTOF - fMassProton * fMassProton, track.pt());
                    // histos.fill(HIST("tracks/evtime/ft0/proton/h2ProtonTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffPr());
                  } else if (track.isEvTimeTOF()) {
                    histos.fill(HIST("tracks/evtime/tof/proton/h2TOFmass2ProtonVsPt"), massTOF * massTOF - fMassProton * fMassProton, track.pt());
                    // histos.fill(HIST("tracks/evtime/tof/proton/h2ProtonTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffPr());
                  } else {
                    histos.fill(HIST("tracks/evtime/fill/proton/h2TOFmass2ProtonVsPt"), massTOF * massTOF - fMassProton * fMassProton, track.pt());
                    // histos.fill(HIST("tracks/evtime/fill/proton/h2ProtonTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffPr());
                  }
                }
              } else {
                if (enablePtSpectra)
                  histos.fill(HIST("tracks/eff/proton/hPtantiPrTOF"), track.pt());
                histos.fill(HIST("tracks/proton/h2TOFmassantiProtonVsPt"), massTOF, track.pt());
                histos.fill(HIST("tracks/proton/h2TOFmass2antiProtonVsPt"), massTOF * massTOF - fMassProton * fMassProton, track.pt());
                if (enableBetaCut && (track.beta() > betaCut)) {
                  histos.fill(HIST("tracks/proton/h2TOFmassantiProtonVsPt_BetaCut"), massTOF, track.pt());
                  histos.fill(HIST("tracks/proton/h2TOFmass2antiProtonVsPt_BetaCut"), massTOF * massTOF - fMassProton * fMassProton, track.pt());
                }
                if (enableExpSignalTOF)
                  histos.fill(HIST("tracks/proton/h2antiProtonTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffPr());
                if (enableNucleiHardCut && (std::abs(track.tpcNSigmaKa()) > 2) && (std::abs(track.tpcNSigmaDe()) > 2)) {
                  histos.fill(HIST("tracks/proton/hc/h2TOFmass2antiProtonVsPt_hard"), massTOF * massTOF - fMassProton * fMassProton, track.pt());
                }
                if (enableEvTimeSplitting) {
                  if (track.isEvTimeTOF() && track.isEvTimeT0AC()) {
                    histos.fill(HIST("tracks/evtime/ft0tof/proton/h2TOFmass2antiProtonVsPt"), massTOF * massTOF - fMassProton * fMassProton, track.pt());
                    // histos.fill(HIST("tracks/evtime/ft0tof/proton/h2antiProtonTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffPr());
                  } else if (track.isEvTimeT0AC()) {
                    histos.fill(HIST("tracks/evtime/ft0/proton/h2TOFmass2antiProtonVsPt"), massTOF * massTOF - fMassProton * fMassProton, track.pt());
                    // histos.fill(HIST("tracks/evtime/ft0/proton/h2antiProtonTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffPr());
                  } else if (track.isEvTimeTOF()) {
                    histos.fill(HIST("tracks/evtime/tof/proton/h2TOFmass2antiProtonVsPt"), massTOF * massTOF - fMassProton * fMassProton, track.pt());
                    // histos.fill(HIST("tracks/evtime/tof/proton/h2antiProtonTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffPr());
                  } else {
                    histos.fill(HIST("tracks/evtime/fill/proton/h2TOFmass2antiProtonVsPt"), massTOF * massTOF - fMassProton * fMassProton, track.pt());
                    // histos.fill(HIST("tracks/evtime/fill/proton/h2antiProtonTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffPr());
                  }
                }
              }
            }
          }
          if (enableDe) {
            // if ((((!enableStrongCut) && (std::abs(track.tpcNSigmaDe()) < nsigmaTPCDe)) || ((enableStrongCut) && (std::abs(track.tpcNSigmaPr()) >= nsigmaTPCStrongCut))) && (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Deuteron))) < yCut)) {
            if ((std::abs(track.tpcNSigmaDe()) < nsigmaTPCDe) && deRapCut) {
              if (track.sign() > 0) {
                if (enablePtSpectra)
                  histos.fill(HIST("tracks/eff/deuteron/hPtDeTOF"), track.pt());
                histos.fill(HIST("tracks/deuteron/h2TOFmassDeuteronVsPt"), massTOF, track.pt());
                histos.fill(HIST("tracks/deuteron/h2TOFmass2DeuteronVsPt"), massTOF * massTOF - fMassDeuteron * fMassDeuteron, track.pt());
                if (enableBetaCut && (track.beta() > betaCut)) {
                  histos.fill(HIST("tracks/deuteron/h2TOFmassDeuteronVsPt_BetaCut"), massTOF, track.pt());
                  histos.fill(HIST("tracks/deuteron/h2TOFmass2DeuteronVsPt_BetaCut"), massTOF * massTOF - fMassDeuteron * fMassDeuteron, track.pt());
                }
                if (enableExpSignalTOF)
                  histos.fill(HIST("tracks/deuteron/h2DeuteronTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffDe());
                if (enableNucleiHardCut && (std::abs(track.tpcNSigmaPr()) > 2) && (std::abs(track.tpcNSigmaTr()) > 2)) {
                  histos.fill(HIST("tracks/deuteron/hc/h2TOFmass2DeuteronVsPt_hard"), massTOF * massTOF - fMassDeuteron * fMassDeuteron, track.pt());
                }
                if (enableEvTimeSplitting) {
                  if (track.isEvTimeTOF() && track.isEvTimeT0AC()) {
                    histos.fill(HIST("tracks/evtime/ft0tof/deuteron/h2TOFmass2DeuteronVsPt"), massTOF * massTOF - fMassDeuteron * fMassDeuteron, track.pt());
                    // histos.fill(HIST("tracks/evtime/ft0tof/deuteron/h2DeuteronTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffDe());
                  } else if (track.isEvTimeT0AC()) {
                    histos.fill(HIST("tracks/evtime/ft0/deuteron/h2TOFmass2DeuteronVsPt"), massTOF * massTOF - fMassDeuteron * fMassDeuteron, track.pt());
                    // histos.fill(HIST("tracks/evtime/ft0/deuteron/h2DeuteronTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffDe());
                  } else if (track.isEvTimeTOF()) {
                    histos.fill(HIST("tracks/evtime/tof/deuteron/h2TOFmass2DeuteronVsPt"), massTOF * massTOF - fMassDeuteron * fMassDeuteron, track.pt());
                    // histos.fill(HIST("tracks/evtime/tof/deuteron/h2DeuteronTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffDe());
                  } else {
                    histos.fill(HIST("tracks/evtime/fill/deuteron/h2TOFmass2DeuteronVsPt"), massTOF * massTOF - fMassDeuteron * fMassDeuteron, track.pt());
                    // histos.fill(HIST("tracks/evtime/fill/deuteron/h2DeuteronTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffDe());
                  }
                }

              } else {
                if (enablePtSpectra)
                  histos.fill(HIST("tracks/eff/deuteron/hPtantiDeTOF"), track.pt());
                histos.fill(HIST("tracks/deuteron/h2TOFmassantiDeuteronVsPt"), massTOF, track.pt());
                histos.fill(HIST("tracks/deuteron/h2TOFmass2antiDeuteronVsPt"), massTOF * massTOF - fMassDeuteron * fMassDeuteron, track.pt());
                if (enableBetaCut && (track.beta() > betaCut)) {
                  histos.fill(HIST("tracks/deuteron/h2TOFmassantiDeuteronVsPt_BetaCut"), massTOF, track.pt());
                  histos.fill(HIST("tracks/deuteron/h2TOFmass2antiDeuteronVsPt_BetaCut"), massTOF * massTOF - fMassDeuteron * fMassDeuteron, track.pt());
                }
                if (enableExpSignalTOF)
                  histos.fill(HIST("tracks/deuteron/h2antiDeuteronTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffDe());
                if (enableNucleiHardCut && (std::abs(track.tpcNSigmaPr()) > 2) && (std::abs(track.tpcNSigmaTr()) > 2)) {
                  histos.fill(HIST("tracks/deuteron/hc/h2TOFmass2antiDeuteronVsPt_hard"), massTOF * massTOF - fMassDeuteron * fMassDeuteron, track.pt());
                }
                if (enableEvTimeSplitting) {
                  if (track.isEvTimeTOF() && track.isEvTimeT0AC()) {
                    histos.fill(HIST("tracks/evtime/ft0tof/deuteron/h2TOFmass2antiDeuteronVsPt"), massTOF * massTOF - fMassDeuteron * fMassDeuteron, track.pt());
                    // histos.fill(HIST("tracks/evtime/ft0tof/deuteron/h2antiDeuteronTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffDe());
                  } else if (track.isEvTimeT0AC()) {
                    histos.fill(HIST("tracks/evtime/ft0/deuteron/h2TOFmass2antiDeuteronVsPt"), massTOF * massTOF - fMassDeuteron * fMassDeuteron, track.pt());
                    // histos.fill(HIST("tracks/evtime/ft0/deuteron/h2antiDeuteronTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffDe());
                  } else if (track.isEvTimeTOF()) {
                    histos.fill(HIST("tracks/evtime/tof/deuteron/h2TOFmass2antiDeuteronVsPt"), massTOF * massTOF - fMassDeuteron * fMassDeuteron, track.pt());
                    // histos.fill(HIST("tracks/evtime/tof/deuteron/h2antiDeuteronTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffDe());
                  } else {
                    histos.fill(HIST("tracks/evtime/fill/deuteron/h2TOFmass2antiDeuteronVsPt"), massTOF * massTOF - fMassDeuteron * fMassDeuteron, track.pt());
                    // histos.fill(HIST("tracks/evtime/fill/deuteron/h2antiDeuteronTOFExpSignalDiffVsPtCut"), track.pt(), track.tofExpSignalDiffDe());
                  }
                }
              }
            }
          }

          if (enableTr) {
            // if ((((!enableStrongCut) && (std::abs(track.tpcNSigmaTr()) < nsigmaTPCTr)) || ((enableStrongCut) && (std::abs(track.tpcNSigmaPr()) >= nsigmaTPCStrongCut))) && (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Triton))) < yCut)) {
            if ((isTriton) && (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Triton))) < yCut)) {
              if (track.sign() > 0) {
                if (enablePtSpectra)
                  histos.fill(HIST("tracks/eff/triton/hPtTrTOF"), track.pt());
                histos.fill(HIST("tracks/triton/h2TOFmassTritonVsPt"), massTOF, track.pt());
                histos.fill(HIST("tracks/triton/h2TOFmass2TritonVsPt"), massTOF * massTOF - fMassTriton * fMassTriton, track.pt());
                if (enableBetaCut && (track.beta() > betaCut)) {
                  histos.fill(HIST("tracks/triton/h2TOFmassTritonVsPt_BetaCut"), massTOF, track.pt());
                  histos.fill(HIST("tracks/triton/h2TOFmass2TritonVsPt_BetaCut"), massTOF * massTOF - fMassTriton * fMassTriton, track.pt());
                }
                if (enableNucleiHardCut && (std::abs(track.tpcNSigmaDe()) > 2) && (std::abs(track.tpcNSigmaHe()) > 2)) {
                  histos.fill(HIST("tracks/triton/hc/h2TOFmass2TritonVsPt_hard"), massTOF * massTOF - fMassTriton * fMassTriton, track.pt());
                }
              } else {
                if (enablePtSpectra)
                  histos.fill(HIST("tracks/eff/triton/hPtantiTrTOF"), track.pt());
                histos.fill(HIST("tracks/triton/h2TOFmassantiTritonVsPt"), massTOF, track.pt());
                histos.fill(HIST("tracks/triton/h2TOFmass2antiTritonVsPt"), massTOF * massTOF - fMassTriton * fMassTriton, track.pt());
                if (enableBetaCut && (track.beta() > betaCut)) {
                  histos.fill(HIST("tracks/triton/h2TOFmassantiTritonVsPt_BetaCut"), massTOF, track.pt());
                  histos.fill(HIST("tracks/triton/h2TOFmass2antiTritonVsPt_BetaCut"), massTOF * massTOF - fMassTriton * fMassTriton, track.pt());
                }
                if (enableNucleiHardCut && (std::abs(track.tpcNSigmaDe()) > 2) && (std::abs(track.tpcNSigmaHe()) > 2)) {
                  histos.fill(HIST("tracks/triton/hc/h2TOFmass2antiTritonVsPt_hard"), massTOF * massTOF - fMassTriton * fMassTriton, track.pt());
                }
              }
            }
          }
          if (enableHe) {
            // if ((((!enableStrongCut) && (std::abs(track.tpcNSigmaHe()) < nsigmaTPCHe)) || ((enableStrongCut) && (std::abs(track.tpcNSigmaPr()) >= nsigmaTPCStrongCut))) && (TMath::Abs(track.rapidity(o2::track::PID::getMass2Z(o2::track::PID::Helium3))) < yCut)) {
            if (rapDebug && (std::abs(track.tpcNSigmaHe()) < nsigmaTPCHe)) {
              if (track.sign() > 0)
                histos.fill(HIST("tracks/helium/h2TOFmassHeliumVsPt_NORAPIDITYCUT"), 2.f * massTOF, hePt);
              else
                histos.fill(HIST("tracks/helium/h2TOFmassantiHeliumVsPt_NORAPIDITYCUT"), 2.f * massTOF, hePt);
            }
            if ((std::abs(track.tpcNSigmaHe()) < nsigmaTPCHe) && heRapCut) {
              if (track.sign() > 0) {
                if (enablePtSpectra)
                  histos.fill(HIST("tracks/eff/helium/hPtHeTOF"), hePt);
                histos.fill(HIST("tracks/helium/h2TOFmassHeliumVsPt"), 2.f * massTOF, hePt);
                histos.fill(HIST("tracks/helium/h2TOFmass2HeliumVsPt"), 2.f * massTOF * 2.f * massTOF - fMassHelium * fMassHelium, hePt);
                if (enableBetaCut && (track.beta() > betaCut)) {
                  histos.fill(HIST("tracks/helium/h2TOFmassHeliumVsPt_BetaCut"), 2.f * massTOF, hePt);
                  histos.fill(HIST("tracks/helium/h2TOFmass2HeliumVsPt_BetaCut"), 2.f * massTOF * 2.f * massTOF - fMassHelium * fMassHelium, hePt);
                }
                if (enableExpSignalTOF)
                  histos.fill(HIST("tracks/helium/h2HeliumTOFExpSignalDiffVsPtCut"), hePt, track.tofExpSignalDiffHe());
                if (enableNucleiHardCut && (std::abs(track.tpcNSigmaTr()) > 2)) {
                  histos.fill(HIST("tracks/helium/hc/h2TOFmass2HeliumVsPt_hard"), 2.f * massTOF * 2.f * massTOF - fMassHelium * fMassHelium, hePt);
                }
              } else {
                if (enablePtSpectra)
                  histos.fill(HIST("tracks/eff/helium/hPtantiHeTOF"), hePt);
                histos.fill(HIST("tracks/helium/h2TOFmassantiHeliumVsPt"), 2.f * massTOF, hePt);
                histos.fill(HIST("tracks/helium/h2TOFmass2antiHeliumVsPt"), 2.f * massTOF * 2.f * massTOF - fMassHelium * fMassHelium, hePt);
                if (enableBetaCut && (track.beta() > betaCut)) {
                  histos.fill(HIST("tracks/helium/h2TOFmassantiHeliumVsPt_BetaCut"), 2.f * massTOF, hePt);
                  histos.fill(HIST("tracks/helium/h2TOFmass2antiHeliumVsPt_BetaCut"), 2.f * massTOF * 2.f * massTOF - fMassHelium * fMassHelium, hePt);
                }
                if (enableExpSignalTOF)
                  histos.fill(HIST("tracks/helium/h2antiHeliumTOFExpSignalDiffVsPtCut"), hePt, track.tofExpSignalDiffHe());
                if (enableNucleiHardCut && (std::abs(track.tpcNSigmaTr()) > 2)) {
                  histos.fill(HIST("tracks/helium/hc/h2TOFmass2antiHeliumVsPt_hard"), 2.f * massTOF * 2.f * massTOF - fMassHelium * fMassHelium, hePt);
                }
              }
            }
          }
        }
      }

      if constexpr (IsMC) {
        bool isPhysPrim = false;
        bool isProdByGen = false;

        // PID
        int pdgCode = 0;
        if constexpr (IsFilteredData) {
          isPhysPrim = track.isPhysicalPrimary();
          isProdByGen = track.producedByGenerator();
          pdgCode = track.pdgCode();
        } else {
          if (!track.has_mcParticle()) {
            continue;
          }
          isPhysPrim = track.mcParticle().isPhysicalPrimary();
          isProdByGen = track.mcParticle().producedByGenerator();
          pdgCode = track.mcParticle().pdgCode();
        }
        switch (pdgCode) {
          case PDGProton:
            if (enablePr) {
              histos.fill(HIST("tracks/proton/h1ProtonSpectraTrue"), track.pt());
              histos.fill(HIST("tracks/proton/dca/after/hDCAxyVsPtProtonTrue"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/proton/dca/after/hDCAzVsPtProtonTrue"), track.pt(), track.dcaZ());
              if (std::abs(track.tpcNSigmaPr()) < nsigmaTPCPr) {
                histos.fill(HIST("tracks/proton/h1ProtonSpectraTrueWPID"), track.pt());
              }
              if (isPhysPrim) {
                histos.fill(HIST("tracks/proton/h1ProtonSpectraTruePrim"), track.pt());
                histos.fill(HIST("tracks/proton/dca/after/hDCAxyVsPtProtonTruePrim"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/proton/dca/after/hDCAzVsPtProtonTruePrim"), track.pt(), track.dcaZ());
              }
              if (!isPhysPrim && isProdByGen) {
                histos.fill(HIST("tracks/proton/h1ProtonSpectraTrueSec"), track.pt());
                histos.fill(HIST("tracks/proton/dca/after/hDCAxyVsPtProtonTrueSec"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/proton/dca/after/hDCAzVsPtProtonTrueSec"), track.pt(), track.dcaZ());
              }
              if (!isPhysPrim && !isProdByGen) {
                histos.fill(HIST("tracks/proton/h1ProtonSpectraTrueTransport"), track.pt());
                histos.fill(HIST("tracks/proton/dca/after/hDCAxyVsPtProtonTrueTransport"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/proton/dca/after/hDCAzVsPtProtonTrueTransport"), track.pt(), track.dcaZ());
              }
            }
            break;
          case -PDGProton:
            if (enablePr) {
              histos.fill(HIST("tracks/proton/h1antiProtonSpectraTrue"), track.pt());
              histos.fill(HIST("tracks/proton/dca/after/hDCAxyVsPtantiProtonTrue"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/proton/dca/after/hDCAzVsPtantiProtonTrue"), track.pt(), track.dcaZ());
              if (std::abs(track.tpcNSigmaPr()) < nsigmaTPCPr) {
                histos.fill(HIST("tracks/proton/h1antiProtonSpectraTrueWPID"), track.pt());
              }
              if (isPhysPrim) {
                histos.fill(HIST("tracks/proton/h1antiProtonSpectraTruePrim"), track.pt());
                histos.fill(HIST("tracks/proton/dca/after/hDCAxyVsPtantiProtonTruePrim"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/proton/dca/after/hDCAzVsPtantiProtonTruePrim"), track.pt(), track.dcaZ());
              }
              if (!isPhysPrim && isProdByGen) {
                histos.fill(HIST("tracks/proton/h1antiProtonSpectraTrueSec"), track.pt());
                histos.fill(HIST("tracks/proton/dca/after/hDCAxyVsPtantiProtonTrueSec"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/proton/dca/after/hDCAzVsPtantiProtonTrueSec"), track.pt(), track.dcaZ());
              }
              if (!isPhysPrim && !isProdByGen) {
                histos.fill(HIST("tracks/proton/h1antiProtonSpectraTrueTransport"), track.pt());
                histos.fill(HIST("tracks/proton/dca/after/hDCAxyVsPtantiProtonTrueTransport"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/proton/dca/after/hDCAzVsPtantiProtonTrueTransport"), track.pt(), track.dcaZ());
              }
            }
            break;
          case PDGDeuteron:
            if (enableDe) {
              histos.fill(HIST("tracks/deuteron/h1DeuteronSpectraTrue"), track.pt());
              histos.fill(HIST("tracks/deuteron/dca/after/hDCAxyVsPtDeuteronTrue"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/deuteron/dca/after/hDCAzVsPtDeuteronTrue"), track.pt(), track.dcaZ());
              if (std::abs(track.tpcNSigmaDe()) < nsigmaTPCDe) {
                histos.fill(HIST("tracks/deuteron/h1DeuteronSpectraTrueWPID"), track.pt());
              }
              if (isPhysPrim) {
                histos.fill(HIST("tracks/deuteron/h1DeuteronSpectraTruePrim"), track.pt());
                histos.fill(HIST("tracks/deuteron/dca/after/hDCAxyVsPtDeuteronTruePrim"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/deuteron/dca/after/hDCAzVsPtDeuteronTruePrim"), track.pt(), track.dcaZ());
              }
              if (!isPhysPrim && isProdByGen) {
                histos.fill(HIST("tracks/deuteron/h1DeuteronSpectraTrueSec"), track.pt());
                histos.fill(HIST("tracks/deuteron/dca/after/hDCAxyVsPtDeuteronTrueSec"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/deuteron/dca/after/hDCAzVsPtDeuteronTrueSec"), track.pt(), track.dcaZ());
              }
              if (!isPhysPrim && !isProdByGen) {
                histos.fill(HIST("tracks/deuteron/h1DeuteronSpectraTrueTransport"), track.pt());
                histos.fill(HIST("tracks/deuteron/dca/after/hDCAxyVsPtDeuteronTrueTransport"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/deuteron/dca/after/hDCAzVsPtDeuteronTrueTransport"), track.pt(), track.dcaZ());
              }
            }
            break;
          case -PDGDeuteron:
            if (enableDe) {
              histos.fill(HIST("tracks/deuteron/h1antiDeuteronSpectraTrue"), track.pt());
              histos.fill(HIST("tracks/deuteron/dca/after/hDCAxyVsPtantiDeuteronTrue"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/deuteron/dca/after/hDCAzVsPtantiDeuteronTrue"), track.pt(), track.dcaZ());
              if (std::abs(track.tpcNSigmaDe()) < nsigmaTPCDe) {
                histos.fill(HIST("tracks/deuteron/h1antiDeuteronSpectraTrueWPID"), track.pt());
              }
              if (isPhysPrim) {
                histos.fill(HIST("tracks/deuteron/h1antiDeuteronSpectraTruePrim"), track.pt());
                histos.fill(HIST("tracks/deuteron/dca/after/hDCAxyVsPtantiDeuteronTruePrim"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/deuteron/dca/after/hDCAzVsPtantiDeuteronTruePrim"), track.pt(), track.dcaZ());
              }
              if (!isPhysPrim && isProdByGen) {
                histos.fill(HIST("tracks/deuteron/h1antiDeuteronSpectraTrueSec"), track.pt());
                histos.fill(HIST("tracks/deuteron/dca/after/hDCAxyVsPtantiDeuteronTrueSec"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/deuteron/dca/after/hDCAzVsPtantiDeuteronTrueSec"), track.pt(), track.dcaZ());
              }
              if (!isPhysPrim && !isProdByGen) {
                histos.fill(HIST("tracks/deuteron/h1antiDeuteronSpectraTrueTransport"), track.pt());
                histos.fill(HIST("tracks/deuteron/dca/after/hDCAxyVsPtantiDeuteronTrueTransport"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/deuteron/dca/after/hDCAzVsPtantiDeuteronTrueTransport"), track.pt(), track.dcaZ());
              }
            }
            break;
          case PDGTriton:
            if (enableTr) {
              histos.fill(HIST("tracks/triton/h1TritonSpectraTrue"), track.pt());
              histos.fill(HIST("tracks/triton/dca/after/hDCAxyVsPtTritonTrue"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/triton/dca/after/hDCAzVsPtTritonTrue"), track.pt(), track.dcaZ());
              if (isPhysPrim) {
                histos.fill(HIST("tracks/triton/h1TritonSpectraTruePrim"), track.pt());
                histos.fill(HIST("tracks/triton/dca/after/hDCAxyVsPtTritonTruePrim"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/triton/dca/after/hDCAzVsPtTritonTruePrim"), track.pt(), track.dcaZ());
              }
              if (!isPhysPrim && isProdByGen) {
                histos.fill(HIST("tracks/triton/h1TritonSpectraTrueSec"), track.pt());
                histos.fill(HIST("tracks/triton/dca/after/hDCAxyVsPtTritonTrueSec"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/triton/dca/after/hDCAzVsPtTritonTrueSec"), track.pt(), track.dcaZ());
              }
              if (!isPhysPrim && !isProdByGen) {
                histos.fill(HIST("tracks/triton/h1TritonSpectraTrueTransport"), track.pt());
                histos.fill(HIST("tracks/triton/dca/after/hDCAxyVsPtTritonTrueTransport"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/triton/dca/after/hDCAzVsPtTritonTrueTransport"), track.pt(), track.dcaZ());
              }
            }
            break;
          case -PDGTriton:
            if (enableTr) {
              histos.fill(HIST("tracks/triton/h1antiTritonSpectraTrue"), track.pt());
              histos.fill(HIST("tracks/triton/dca/after/hDCAxyVsPtantiTritonTrue"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/triton/dca/after/hDCAzVsPtantiTritonTrue"), track.pt(), track.dcaZ());
              if (isPhysPrim) {
                histos.fill(HIST("tracks/triton/h1antiTritonSpectraTruePrim"), track.pt());
                histos.fill(HIST("tracks/triton/dca/after/hDCAxyVsPtantiTritonTruePrim"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/triton/dca/after/hDCAzVsPtantiTritonTruePrim"), track.pt(), track.dcaZ());
              }
              if (!isPhysPrim && isProdByGen) {
                histos.fill(HIST("tracks/triton/h1antiTritonSpectraTrueSec"), track.pt());
                histos.fill(HIST("tracks/triton/dca/after/hDCAxyVsPtantiTritonTrueSec"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/triton/dca/after/hDCAzVsPtantiTritonTrueSec"), track.pt(), track.dcaZ());
              }
              if (!isPhysPrim && !isProdByGen) {
                histos.fill(HIST("tracks/triton/h1antiTritonSpectraTrueTransport"), track.pt());
                histos.fill(HIST("tracks/triton/dca/after/hDCAxyVsPtantiTritonTrueTransport"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/triton/dca/after/hDCAzVsPtantiTritonTrueTransport"), track.pt(), track.dcaZ());
              }
            }
            break;
          case PDGHelium:
            if (enableHe) {
              histos.fill(HIST("tracks/helium/h1HeliumSpectraTrue"), hePt);
              histos.fill(HIST("tracks/helium/dca/after/hDCAxyVsPtHeliumTrue"), hePt, track.dcaXY());
              histos.fill(HIST("tracks/helium/dca/after/hDCAzVsPtHeliumTrue"), hePt, track.dcaZ());
              if (std::abs(track.tpcNSigmaHe()) < nsigmaTPCHe) {
                histos.fill(HIST("tracks/helium/h1HeliumSpectraTrueWPID"), hePt);
              }
              if (isPhysPrim) {
                histos.fill(HIST("tracks/helium/h1HeliumSpectraTruePrim"), hePt);
                histos.fill(HIST("tracks/helium/dca/after/hDCAxyVsPtHeliumTruePrim"), hePt, track.dcaXY());
                histos.fill(HIST("tracks/helium/dca/after/hDCAzVsPtHeliumTruePrim"), hePt, track.dcaZ());
              }
              if (!isPhysPrim && isProdByGen) {
                histos.fill(HIST("tracks/helium/h1HeliumSpectraTrueSec"), hePt);
                histos.fill(HIST("tracks/helium/dca/after/hDCAxyVsPtHeliumTrueSec"), hePt, track.dcaXY());
                histos.fill(HIST("tracks/helium/dca/after/hDCAzVsPtHeliumTrueSec"), hePt, track.dcaZ());
              }
              if (!isPhysPrim && !isProdByGen) {
                histos.fill(HIST("tracks/helium/h1HeliumSpectraTrueTransport"), hePt);
                histos.fill(HIST("tracks/helium/dca/after/hDCAxyVsPtHeliumTrueTransport"), hePt, track.dcaXY());
                histos.fill(HIST("tracks/helium/dca/after/hDCAzVsPtHeliumTrueTransport"), hePt, track.dcaZ());
              }
            }
            break;
          case -PDGHelium:
            if (enableHe) {
              histos.fill(HIST("tracks/helium/h1antiHeliumSpectraTrue"), hePt);
              histos.fill(HIST("tracks/helium/dca/after/hDCAxyVsPtantiHeliumTrue"), hePt, track.dcaXY());
              histos.fill(HIST("tracks/helium/dca/after/hDCAzVsPtantiHeliumTrue"), hePt, track.dcaZ());
              if (std::abs(track.tpcNSigmaHe()) < nsigmaTPCHe) {
                histos.fill(HIST("tracks/helium/h1antiHeliumSpectraTrueWPID"), hePt);
              }
              if (isPhysPrim) {
                histos.fill(HIST("tracks/helium/h1antiHeliumSpectraTruePrim"), hePt);
                histos.fill(HIST("tracks/helium/dca/after/hDCAxyVsPtantiHeliumTruePrim"), hePt, track.dcaXY());
                histos.fill(HIST("tracks/helium/dca/after/hDCAzVsPtantiHeliumTruePrim"), hePt, track.dcaZ());
              }
              if (!isPhysPrim && isProdByGen) {
                histos.fill(HIST("tracks/helium/h1antiHeliumSpectraTrueSec"), hePt);
                histos.fill(HIST("tracks/helium/dca/after/hDCAxyVsPtantiHeliumTrueSec"), hePt, track.dcaXY());
                histos.fill(HIST("tracks/helium/dca/after/hDCAzVsPtantiHeliumTrueSec"), hePt, track.dcaZ());
              }
              if (!isPhysPrim && !isProdByGen) {
                histos.fill(HIST("tracks/helium/h1antiHeliumSpectraTrueTransport"), hePt);
                histos.fill(HIST("tracks/helium/dca/after/hDCAxyVsPtantiHeliumTrueTransport"), hePt, track.dcaXY());
                histos.fill(HIST("tracks/helium/dca/after/hDCAzVsPtantiHeliumTrueTransport"), hePt, track.dcaZ());
              }
            }
            break;
          case PDGAlpha:
            if (enableAl) {
              histos.fill(HIST("tracks/alpha/h1AlphaSpectraTrue"), track.pt());
              histos.fill(HIST("tracks/alpha/dca/after/hDCAxyVsPtAlphaTrue"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/alpha/dca/after/hDCAzVsPtAlphaTrue"), track.pt(), track.dcaZ());
              if (isPhysPrim) {
                histos.fill(HIST("tracks/alpha/h1AlphaSpectraTruePrim"), track.pt());
                histos.fill(HIST("tracks/alpha/dca/after/hDCAxyVsPtAlphaTruePrim"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/alpha/dca/after/hDCAzVsPtAlphaTruePrim"), track.pt(), track.dcaZ());
              }
              if (!isPhysPrim && isProdByGen) {
                histos.fill(HIST("tracks/alpha/h1AlphaSpectraTrueSec"), track.pt());
                histos.fill(HIST("tracks/alpha/dca/after/hDCAxyVsPtAlphaTrueSec"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/alpha/dca/after/hDCAzVsPtAlphaTrueSec"), track.pt(), track.dcaZ());
              }
              if (!isPhysPrim && !isProdByGen) {
                histos.fill(HIST("tracks/alpha/h1AlphaSpectraTrueTransport"), track.pt());
                histos.fill(HIST("tracks/alpha/dca/after/hDCAxyVsPtAlphaTrueTransport"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/alpha/dca/after/hDCAzVsPtAlphaTrueTransport"), track.pt(), track.dcaZ());
              }
            }
            break;
          case -PDGAlpha:
            if (enableAl) {
              histos.fill(HIST("tracks/alpha/h1antiAlphaSpectraTrue"), track.pt());
              histos.fill(HIST("tracks/alpha/dca/after/hDCAxyVsPtantiAlphaTrue"), track.pt(), track.dcaXY());
              histos.fill(HIST("tracks/alpha/dca/after/hDCAzVsPtantiAlphaTrue"), track.pt(), track.dcaZ());
              if (isPhysPrim) {
                histos.fill(HIST("tracks/alpha/h1antiAlphaSpectraTruePrim"), track.pt());
                histos.fill(HIST("tracks/alpha/dca/after/hDCAxyVsPtantiAlphaTruePrim"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/alpha/dca/after/hDCAzVsPtantiAlphaTruePrim"), track.pt(), track.dcaZ());
              }
              if (!isPhysPrim && isProdByGen) {
                histos.fill(HIST("tracks/alpha/h1antiAlphaSpectraTrueSec"), track.pt());
                histos.fill(HIST("tracks/alpha/dca/after/hDCAxyVsPtantiAlphaTrueSec"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/alpha/dca/after/hDCAzVsPtantiAlphaTrueSec"), track.pt(), track.dcaZ());
              }
              if (!isPhysPrim && !isProdByGen) {
                histos.fill(HIST("tracks/alpha/h1antiAlphaSpectraTrueTransport"), track.pt());
                histos.fill(HIST("tracks/alpha/dca/after/hDCAxyVsPtantiAlphaTrueTransport"), track.pt(), track.dcaXY());
                histos.fill(HIST("tracks/alpha/dca/after/hDCAzVsPtantiAlphaTrueTransport"), track.pt(), track.dcaZ());
              }
            }
            break;
          default:
            break;
        }
      }
    }
  }

  using EventCandidates = soa::Join<aod::Collisions, aod::EvSels, aod::Mults>;
  using TrackCandidates0 = soa::Join<aod::Tracks, aod::TracksExtra, aod::TracksDCA, aod::TrackSelection,
                                     aod::pidTOFbeta, aod::TOFSignal, aod::pidEvTimeFlags,
                                     aod::pidTPCFullPi, aod::pidTOFFullPi,
                                     aod::pidTPCFullKa, aod::pidTOFFullKa,
                                     aod::pidTOFFullPr,
                                     aod::pidTOFFullDe,
                                     aod::pidTOFFullTr,
                                     aod::pidTOFFullHe,
                                     aod::pidTOFFullAl>;
  using TrackCandidates = soa::Join<TrackCandidates0,
                                    aod::pidTPCFullPr,
                                    aod::pidTPCFullDe,
                                    aod::pidTPCFullTr,
                                    aod::pidTPCFullHe,
                                    aod::pidTPCFullAl>;

  using TrackCandidatesLfPid = soa::Join<TrackCandidates0,
                                         aod::pidTPCLfFullPr,
                                         aod::pidTPCLfFullDe,
                                         aod::pidTPCLfFullTr,
                                         aod::pidTPCLfFullHe,
                                         aod::pidTPCLfFullAl>;

  //////////
  // Data //
  //////////

  // Process function that runs on the original AO2D
  void processData(EventCandidates::iterator const& event,
                   TrackCandidates const& tracks)
  {
    fillHistograms<false /*MC*/, false /*Filtered*/>(event, tracks, true /*dummy*/);
  } // CLOSING PROCESS DATA
  PROCESS_SWITCH(LFNucleiBATask, processData, "process data", true);

  // Process function that runs on the original AO2D
  void processDataLfPid(EventCandidates::iterator const& event,
                        TrackCandidatesLfPid const& tracks)
  {
    fillHistograms<false /*MC*/, false /*Filtered*/>(event, tracks, true /*dummy*/);
  } // CLOSING PROCESS DATA
  PROCESS_SWITCH(LFNucleiBATask, processDataLfPid, "process data with LF PID", false);

  // Process function that runs on the filtered data
  void processDataFiltered(o2::aod::LfCandNucleusEvents::iterator const& event,
                           o2::aod::LfCandNucleusFull const& tracks)
  {
    // Runs on data filtered on the fly with LF Tree creator nuclei task
    // Takes as input full AO2Ds
    fillHistograms<false /*MC*/, true /*Filtered*/>(event, tracks, true /*dummy*/);
  } // CLOSING PROCESS DATA ON FILTERED DATA
  PROCESS_SWITCH(LFNucleiBATask, processDataFiltered, "process data on the filtered data", false);

  void processDataLight(o2::aod::LfCandNucleusEvents::iterator const& event,
                        o2::aod::LfCandNucleusDummy const& tracks)
  {
    // Runs on derived tables produced with LF Tree creator nuclei task
    // Takes as input derived trees
    fillHistograms<false /*MC*/, true /*Filtered*/>(event, tracks, true /*dummy*/);
  } // CLOSING PROCESS DATA ON FILTERED DATA
  PROCESS_SWITCH(LFNucleiBATask, processDataLight, "process data on the derived trees", false);

  /////////////
  // MC Reco //
  /////////////

  // Process function that runs on the original AO2D (for the MC)
  void processMCReco(EventCandidates::iterator const& event,
                     soa::Join<TrackCandidates, aod::McTrackLabels> const& tracks,
                     aod::McParticles const& mcParticles)
  {
    fillHistograms<true /*MC*/, false /*Filtered*/>(event, tracks, mcParticles);
  } // CLOSING PROCESS MC RECO
  PROCESS_SWITCH(LFNucleiBATask, processMCReco, "process mc reco", false);

  // Process function that runs on the filtered AO2D (for the MC)
  void processMCRecoFiltered(o2::aod::LfCandNucleusEvents::iterator const& event,
                             soa::Join<o2::aod::LfCandNucleusFull, o2::aod::LfCandNucleusMC> const& tracks)
  {
    fillHistograms<true /*MC*/, true /*Filtered*/>(event, tracks, true /*dummy*/);
  } // CLOSING PROCESS MC RECO ON FILTERED DATA
  PROCESS_SWITCH(LFNucleiBATask, processMCRecoFiltered, "process mc reco on the filtered data", false);

  ////////////
  // MC Gen //
  ////////////

  // LOOP OVER GENERATED MC PARTICLES
  void processMCGen(aod::McCollision const& mcCollision,
                    aod::McParticles& mcParticles)
  {
    histos.fill(HIST("spectraGen/histGenVetxZ"), mcCollision.posZ());
    for (auto& mcParticleGen : mcParticles) {
      if (abs(mcParticleGen.y()) > std::abs(yCut)) {
        continue;
      }

      bool isPhysPrim = mcParticleGen.isPhysicalPrimary();
      bool isProdByGen = mcParticleGen.producedByGenerator();
      if (mcParticleGen.pdgCode() == PDGPion) {
        histos.fill(HIST("spectraGen/pion/histGenPtPion"), mcParticleGen.pt());
        if (isPhysPrim)
          histos.fill(HIST("spectraGen/pion/histGenPtPionPrim"), mcParticleGen.pt());
        if (!isPhysPrim && isProdByGen)
          histos.fill(HIST("spectraGen/pion/histGenPtPionSec"), mcParticleGen.pt());
        if (!isPhysPrim && !isProdByGen)
          histos.fill(HIST("spectraGen/pion/histSecTransportPtPion"), mcParticleGen.pt());
      }
      if (mcParticleGen.pdgCode() == -PDGPion) {
        histos.fill(HIST("spectraGen/pion/histGenPtantiPion"), mcParticleGen.pt());
        if (isPhysPrim)
          histos.fill(HIST("spectraGen/pion/histGenPtantiPionPrim"), mcParticleGen.pt());
        if (!isPhysPrim && isProdByGen)
          histos.fill(HIST("spectraGen/pion/histGenPtantiPionSec"), mcParticleGen.pt());
        if (!isPhysPrim && !isProdByGen)
          histos.fill(HIST("spectraGen/pion/histSecTransportPtantiPion"), mcParticleGen.pt());
      }
      if (mcParticleGen.pdgCode() == PDGKaon) {
        histos.fill(HIST("spectraGen/kaon/histGenPtKaon"), mcParticleGen.pt());
        if (isPhysPrim)
          histos.fill(HIST("spectraGen/kaon/histGenPtKaonPrim"), mcParticleGen.pt());
        if (!isPhysPrim && isProdByGen)
          histos.fill(HIST("spectraGen/kaon/histGenPtKaonSec"), mcParticleGen.pt());
        if (!isPhysPrim && !isProdByGen)
          histos.fill(HIST("spectraGen/kaon/histSecTransportPtKaon"), mcParticleGen.pt());
      }
      if (mcParticleGen.pdgCode() == -PDGKaon) {
        histos.fill(HIST("spectraGen/kaon/histGenPtantiKaon"), mcParticleGen.pt());
        if (isPhysPrim)
          histos.fill(HIST("spectraGen/kaon/histGenPtantiKaonPrim"), mcParticleGen.pt());
        if (!isPhysPrim && isProdByGen)
          histos.fill(HIST("spectraGen/kaon/histGenPtantiKaonSec"), mcParticleGen.pt());
        if (!isPhysPrim && !isProdByGen)
          histos.fill(HIST("spectraGen/kaon/histSecTransportPtantiKaon"), mcParticleGen.pt());
      }
      if (enablePr) {
        if (mcParticleGen.pdgCode() == PDGProton) {
          histos.fill(HIST("spectraGen/proton/histGenPtProton"), mcParticleGen.pt());
          if (isPhysPrim)
            histos.fill(HIST("spectraGen/proton/histGenPtProtonPrim"), mcParticleGen.pt());
          if (!isPhysPrim && isProdByGen)
            histos.fill(HIST("spectraGen/proton/histGenPtProtonSec"), mcParticleGen.pt());
          if (!isPhysPrim && !isProdByGen)
            histos.fill(HIST("spectraGen/proton/histSecTransportPtProton"), mcParticleGen.pt());
        }
        if (mcParticleGen.pdgCode() == -PDGProton) {
          histos.fill(HIST("spectraGen/proton/histGenPtantiProton"), mcParticleGen.pt());
          if (isPhysPrim)
            histos.fill(HIST("spectraGen/proton/histGenPtantiProtonPrim"), mcParticleGen.pt());
          if (!isPhysPrim && isProdByGen)
            histos.fill(HIST("spectraGen/proton/histGenPtantiProtonSec"), mcParticleGen.pt());
          if (!isPhysPrim && !isProdByGen)
            histos.fill(HIST("spectraGen/proton/histSecTransportPtantiProton"), mcParticleGen.pt());
        }
      }
      if (enableDe) {
        if (mcParticleGen.pdgCode() == PDGDeuteron) {
          histos.fill(HIST("spectraGen/deuteron/histGenPtD"), mcParticleGen.pt());
          if (isPhysPrim)
            histos.fill(HIST("spectraGen/deuteron/histGenPtDPrim"), mcParticleGen.pt());
          if (!isPhysPrim && isProdByGen)
            histos.fill(HIST("spectraGen/deuteron/histGenPtDSec"), mcParticleGen.pt());
          if (!isPhysPrim && !isProdByGen)
            histos.fill(HIST("spectraGen/deuteron/histSecTransportPtD"), mcParticleGen.pt());
        }
        if (mcParticleGen.pdgCode() == -PDGDeuteron) {
          histos.fill(HIST("spectraGen/deuteron/histGenPtantiD"), mcParticleGen.pt());
          if (isPhysPrim)
            histos.fill(HIST("spectraGen/deuteron/histGenPtantiDPrim"), mcParticleGen.pt());
          if (!isPhysPrim && isProdByGen)
            histos.fill(HIST("spectraGen/deuteron/histGenPtantiDSec"), mcParticleGen.pt());
          if (!isPhysPrim && !isProdByGen)
            histos.fill(HIST("spectraGen/deuteron/histSecTransportPtantiD"), mcParticleGen.pt());
        }
      }
      if (enableTr) {
        if (mcParticleGen.pdgCode() == PDGTriton) {
          histos.fill(HIST("spectraGen/triton/histGenPtT"), mcParticleGen.pt());
          if (isPhysPrim)
            histos.fill(HIST("spectraGen/triton/histGenPtTPrim"), mcParticleGen.pt());
          if (!isPhysPrim && isProdByGen)
            histos.fill(HIST("spectraGen/triton/histGenPtTSec"), mcParticleGen.pt());
          if (!isPhysPrim && !isProdByGen)
            histos.fill(HIST("spectraGen/triton/histSecTransportPtT"), mcParticleGen.pt());
        }
        if (mcParticleGen.pdgCode() == -PDGTriton) {
          histos.fill(HIST("spectraGen/triton/histGenPtantiT"), mcParticleGen.pt());
          if (isPhysPrim)
            histos.fill(HIST("spectraGen/triton/histGenPtantiTPrim"), mcParticleGen.pt());
          if (!isPhysPrim && isProdByGen)
            histos.fill(HIST("spectraGen/triton/histGenPtantiTSec"), mcParticleGen.pt());
          if (!isPhysPrim && !isProdByGen)
            histos.fill(HIST("spectraGen/triton/histSecTransportPtantiT"), mcParticleGen.pt());
        }
      }
      if (enableHe) {
        if (mcParticleGen.pdgCode() == PDGHelium) {
          histos.fill(HIST("spectraGen/helium/histGenPtHe"), mcParticleGen.pt());
          if (isPhysPrim)
            histos.fill(HIST("spectraGen/helium/histGenPtHePrim"), mcParticleGen.pt());
          if (!isPhysPrim && isProdByGen)
            histos.fill(HIST("spectraGen/helium/histGenPtHeSec"), mcParticleGen.pt());
          if (!isPhysPrim && !isProdByGen)
            histos.fill(HIST("spectraGen/helium/histSecTransportPtHe"), mcParticleGen.pt());
        }
        if (mcParticleGen.pdgCode() == -PDGHelium) {
          histos.fill(HIST("spectraGen/helium/histGenPtantiHe"), mcParticleGen.pt());
          if (isPhysPrim)
            histos.fill(HIST("spectraGen/helium/histGenPtantiHePrim"), mcParticleGen.pt());
          if (!isPhysPrim && isProdByGen)
            histos.fill(HIST("spectraGen/helium/histGenPtantiHeSec"), mcParticleGen.pt());
          if (!isPhysPrim && !isProdByGen)
            histos.fill(HIST("spectraGen/helium/histSecTransportPtantiHe"), mcParticleGen.pt());
        }
      }
      if (enableAl) {
        if (mcParticleGen.pdgCode() == PDGAlpha) {
          histos.fill(HIST("spectraGen/alpha/histGenPtAl"), mcParticleGen.pt());
          if (isPhysPrim)
            histos.fill(HIST("spectraGen/alpha/histGenPtAlPrim"), mcParticleGen.pt());
          if (!isPhysPrim && isProdByGen)
            histos.fill(HIST("spectraGen/alpha/histGenPtAlSec"), mcParticleGen.pt());
          if (!isPhysPrim && !isProdByGen)
            histos.fill(HIST("spectraGen/alpha/histSecTransportPtAl"), mcParticleGen.pt());
        }
        if (mcParticleGen.pdgCode() == -PDGAlpha) {
          histos.fill(HIST("spectraGen/alpha/histGenPtantiAl"), mcParticleGen.pt());
          if (isPhysPrim)
            histos.fill(HIST("spectraGen/alpha/histGenPtantiAlPrim"), mcParticleGen.pt());
          if (!isPhysPrim && isProdByGen)
            histos.fill(HIST("spectraGen/alpha/histGenPtantiAlSec"), mcParticleGen.pt());
          if (!isPhysPrim && !isProdByGen)
            histos.fill(HIST("spectraGen/alpha/histSecTransportPtantiAl"), mcParticleGen.pt());
        }
      }
    }
  } // Close processMCGen
  PROCESS_SWITCH(LFNucleiBATask, processMCGen, "process MC Generated", true);
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  return WorkflowSpec{adaptAnalysisTask<LFNucleiBATask>(cfgc)};
}
