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

#include "Framework/runDataProcessing.h"
#include "Framework/AnalysisTask.h"
#include "Framework/AnalysisDataModel.h"

#include "Common/DataModel/TrackSelectionTables.h"
#include "Common/DataModel/EventSelection.h"

#include "Framework/HistogramRegistry.h"

#include <TH1D.h>
#include "TLorentzVector.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::framework::expressions;

struct UPCSpectraReferenceTask {

  HistogramRegistry histos{"Histos", {}, OutputObjHandlingPolicy::AnalysisObject};

  void init(o2::framework::InitContext&)
  {
    histos.add("fhMass", "Mass of mother", kTH1F, {{500, 0.55, 1.4}});
    histos.add("fhPt", "Pt of mother", kTH1F, {{500, 0., 1.}});
  }

  float mPion = 0.13957;
  Filter trackFilter = (requireGlobalTrackInFilter());

  void process(soa::Join<aod::Collisions, aod::EvSels>::iterator const& col, soa::Filtered<soa::Join<aod::Tracks, aod::TracksExtra, aod::TrackSelection>> const& tracks)
  {
    bool checkV0 = col.selection_bit(kIsBBV0A) || col.selection_bit(kIsBBV0C) || !col.selection_bit(kNoBGV0A) || !col.selection_bit(kNoBGV0C);
    if (checkV0) {
      return;
    }
    bool checkFDD = col.selection_bit(kIsBBFDA) || col.selection_bit(kIsBBFDC) || !col.selection_bit(kNoBGFDA) || !col.selection_bit(kNoBGFDC);
    if (checkFDD) {
      return;
    }
    if (!col.alias_bit(kCUP9)) {
      return;
    }
    if (tracks.size() != 2) {
      return;
    }
    auto first = tracks.begin();
    auto second = first + 1;
    if (first.sign() * second.sign() >= 0) {
      return;
    }
    UChar_t clustermap1 = first.itsClusterMap();
    UChar_t clustermap2 = second.itsClusterMap();
    bool checkClusMap = TESTBIT(clustermap1, 0) && TESTBIT(clustermap1, 1) && TESTBIT(clustermap2, 0) && TESTBIT(clustermap2, 1);
    if (!checkClusMap) {
      return;
    }
    TLorentzVector p1, p2, p;
    p1.SetXYZM(first.px(), first.py(), first.pz(), mPion);
    p2.SetXYZM(second.px(), second.py(), second.pz(), mPion);
    p = p1 + p2;
    histos.fill(HIST("fhPt"), p.Pt());
    float signalTPC1 = first.tpcSignal();
    float signalTPC2 = second.tpcSignal();
    // hSignalTPC1vsSignalTPC2->Fill(signalTPC1, signalTPC2);
    if ((p.Pt() < 0.1) && (signalTPC1 + signalTPC2 < 140.)) {
      histos.fill(HIST("fhMass"), p.M());
    }
  }
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  return WorkflowSpec{
    adaptAnalysisTask<UPCSpectraReferenceTask>(cfgc, TaskName{"upcspectra-task-skim-reference"})};
}
