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

/// \file collisionCuts.h
/// \brief Traditional event selection cuts for O2 analysis
///
/// Simply copied from FemtoDreamCollisionSelection.h
/// original author: Laura Serksnyte, TU München
///
/// \author Bong-Hwi Lim <bong-hwi.lim@cern.ch>

#ifndef PWGLF_UTILS_COLLISIONCUTS_H_
#define PWGLF_UTILS_COLLISIONCUTS_H_

#include "Framework/HistogramRegistry.h"
#include "Framework/Logger.h"

using namespace o2::framework;

namespace o2::analysis
{

class CollisonCuts
{
 public:
  virtual ~CollisonCuts() = default;

  /// \brief Pass the selection criteria to the class
  /// \param zvtxMax Maximal value of the z-vertex
  /// \param checkTrigger whether or not to check for the trigger alias
  /// \param trig Requested trigger alias
  /// \param checkOffline whether or not to check for offline selection criteria
  void setCuts(float zvtxMax, bool checkTrigger, int trig, bool checkOffline, bool checkRun3)
  {
    mCutsSet = true;
    mZvtxMax = zvtxMax;
    mCheckTrigger = checkTrigger;
    mTrigger = trig;
    mCheckOffline = checkOffline;
    mCheckIsRun3 = checkRun3;
  }

  /// Initializes histograms for the task
  /// \param registry Histogram registry to be passed
  void init(HistogramRegistry* registry)
  {
    if (!mCutsSet) {
      LOGF(error, "Event selection not set - quitting!");
    }
    mHistogramRegistry = registry;
    mHistogramRegistry->add("Event/posZ", "; vtx_{z} (cm); Entries", kTH1F, {{300, -12.5, 12.5}});
    mHistogramRegistry->add("Event/MultFV0M", "; vMultV0M; Entries", kTH1F, {{120, 0, 120}});
    mHistogramRegistry->add("Event/MultFT0M", "; vMultT0M; Entries", kTH1F, {{120, 0, 120}});
    mHistogramRegistry->add("Event/MultFT0C", "; vMultT0C; Entries", kTH1F, {{120, 0, 120}});
    mHistogramRegistry->add("Event/MultFT0A", "; vMultT0A; Entries", kTH1F, {{120, 0, 120}});
    mHistogramRegistry->add("Event/MultTPC", "; vMultTPC; Entries", kTH1F, {{300, 0, 3000}});
  }

  /// Print some debug information
  void printCuts()
  {
    LOGF(info, "Debug information for Collison Cuts \n Max. z-vertex: %f \n Check trigger: %d \n Trigger: %d \n Check offline: %d \n", mZvtxMax, mCheckTrigger, mTrigger, mCheckOffline);
  }

  /// Scan the trigger alias of the event
  void setInitialTriggerScan(bool checkTrigger) { mInitialTriggerScan = checkTrigger; }

  /// Check whether the collisions fulfills the specified selections
  /// \tparam T type of the collision
  /// \param col Collision
  /// \return whether or not the collisions fulfills the specified selections
  template <typename T>
  bool isSelected(T const& col)
  {
    if (std::abs(col.posZ()) > mZvtxMax) {
      LOGF(debug, "Vertex out of range");
      return false;
    }
    if (mCheckIsRun3) { // Run3 case
      if (mCheckOffline && !col.sel8()) {
        LOGF(debug, "Offline selection failed (Run3)");
        return false;
      }
    } else { // Run2 case
      if (mCheckOffline && !col.sel7()) {
        LOGF(debug, "Offline selection failed (sel7)");
        return false;
      }
    }
    if (mCheckTrigger && !col.alias_bit(mTrigger)) {
      LOGF(debug, "Trigger selection failed");
      if (mInitialTriggerScan) { // Print out the trigger bits
        LOGF(debug, "Trigger scan initialized");
        for (int i = 0; i < kNaliases; i++) {
          if (col.alias_bit(i)) {
            LOGF(debug, "Trigger %d fired", i);
          }
        }
        mInitialTriggerScan = false;
      }
      return false;
    }
    return true;
  }

  /// Some basic QA of the event
  /// \tparam T type of the collision
  /// \param col Collision
  template <typename T>
  void fillQA(T const& col)
  {
    if (mHistogramRegistry) {
      mHistogramRegistry->fill(HIST("Event/posZ"), col.posZ());
      mHistogramRegistry->fill(HIST("Event/MultFV0M"), col.multFV0M());
      mHistogramRegistry->fill(HIST("Event/MultFT0M"), col.centFT0M());
      mHistogramRegistry->fill(HIST("Event/MultFT0C"), col.centFT0C());
      mHistogramRegistry->fill(HIST("Event/MultFT0A"), col.centFT0A());
      mHistogramRegistry->fill(HIST("Event/MultTPC"), col.multTPC());
    }
  }

  /// Compute the spherocity of an event
  /// Important here is that the filter on tracks does not interfere here!
  /// In Run 2 we used here global tracks within |eta| < 0.8
  /// \tparam T1 type of the collision
  /// \tparam T2 type of the tracks
  /// \param col Collision
  /// \param tracks All tracks
  /// \return value of the sphericity of the event
  template <typename T1, typename T2>
  float computeSpherocity(T1 const& col, T2 const& tracks)
  {
    int size = tracks.size();
    float Sp = 1;
    for (auto const& trk1 : tracks) {
      float sum1 = 0;
      float phi1 = trk1.phi();
      int ctr = 0;
      for (auto const& trk2 : tracks) {
        ++ctr;
        if (trk1.index() == trk2.index())
          continue;
        float phi2 = trk2.phi();
        sum1 += abs(sin(phi1 - phi2));
      }
      float sph = pow(sum1 / static_cast<float>(size), 2);
      if (sph < Sp) {
        Sp = sph;
      }
    }
    float spherocity = pow(M_PI_2, 2) * Sp;
    return spherocity;
  }

 private:
  HistogramRegistry* mHistogramRegistry = nullptr; ///< For QA output
  bool mCutsSet = false;                           ///< Protection against running without cuts
  bool mCheckTrigger = false;                      ///< Check for trigger
  bool mCheckOffline = false;                      ///< Check for offline criteria (might change)
  bool mCheckIsRun3 = false;                       ///< Check if running on Pilot Beam
  bool mInitialTriggerScan = false;                ///< Check trigger when the event is first selected
  int mTrigger = kINT7;                            ///< Trigger to check for
  float mZvtxMax = 999.f;                          ///< Maximal deviation from nominal z-vertex (cm)
};
} // namespace o2::analysis

#endif // PWGLF_UTILS_COLLISIONCUTS_H_
