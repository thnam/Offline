#!/bin/bash
for mod in "makeSH" "FSHPreStereo" "MakeStrawHitPositions" "MakeStereoHits" "FlagStrawHits" "FlagBkgHits" "TimeClusterFinder" "TCFilter" "PosHelixFinder" "PosHelixFilter" "KSFDeM" "DeMSeedFilter" "KFFDeM" "DeMKalFilter"; do
echo Processing module $mod
sqlite3  -separator "," $1 "SELECT Event, Time, ModuleType FROM TimeModule WHERE ModuleLabel=\"${mod}\";" > $mod.clv
done

