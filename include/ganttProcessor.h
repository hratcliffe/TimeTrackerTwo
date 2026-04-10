#ifndef __ganttProcessor__
#define __ganttProcessor__

#include <map>
#include "idGenerators.h"
#include "dataObjects.h"
/**
 * @brief Process time data
 *
 * Processes the independent timing info on multiple projects into a single format suitable for drawing a Gantt chart, or a stacked-bar-chart.
 * This basically means re-processing each set of slices onto a common set of time-bins, covering the total interval
 */
class ganttProcessor{
    public:
    /**
     * @brief Fix time-bins to cover, at most [start, end).
     *
     * Assumes the selction covers at most one bin too many - i.e only the first and last may exceed the stated envelope.
     * @param input Original list
     * @param start Desired start
     * @param end Desired end
     * @return std::map<proIds::Uuid, projectSliceData>
     */
    static std::map<proIds::Uuid, projectSliceData> envelope(std::map<proIds::Uuid, projectSliceData> & input, timecode start, timecode end){
        for(auto & entry: input){
            if(entry.second.slices.size() == 0) break;
            auto e = entry.second.slices.size()-1;
            if(entry.second.slices[0].start == timecodeNull || entry.second.slices[0].start < start) entry.second.slices[0].start = start;
            if(entry.second.slices[e].end == timecodeNull || entry.second.slices[e].end > end) entry.second.slices[e].end = end;
        }
        return input;
    }
    /**
     * @brief Reprocess onto shared times
     *
     * Takes a map of ids onto time bins and remaps each entry onto the union set of bins (i.e the set of bins that accomodates all entries)
     * @pre All slices have specific (not null) start and end. No slices for a single project overlap.
     * @param input Set to process. 
     * @param fillCumulates Whether to produce the null-uid entry containing totals - requires all totals to not exceed 1. If input contains a NullUid entry and fillCumulates is true, this entry will be clobberred
     * @return std::map<proIds::Uuid, projectSliceData>
     */
    static std::map<proIds::Uuid, projectSliceData> reprocess(std::map<proIds::Uuid, projectSliceData> input, bool fillCumulates=true){
        //Forming the list of all the edges (skipping null)
        std::vector<timecode> edges;
        for(auto & entry: input){
            for(auto & slice : entry.second.slices){
                if(slice.start != timecodeNull) edges.push_back(slice.start);
                if(slice.end != timecodeNull) edges.push_back(slice.end);
            }
        }
        //Sorting the edges
        std::sort(edges.begin(), edges.end());
        auto it = unique(edges.begin(), edges.end());
        edges.resize(std::distance(edges.begin(), it));
        // Re-cut each project onto the new edges
        // Assume we have succeeded, and can thus rely on all previous edges appearing
        std::map<proIds::Uuid, projectSliceData> out;
        //Adding a dummy entry with just the bin defs, all values start at 0.0
        // Then we cumulate the total into these
        projectSliceData dummy;
        if(fillCumulates){
            for(size_t e = 0; e< edges.size()-1; e++){
                dummy.slices.push_back({edges[e], edges[e+1], eb_float{}});
            }
            dummy.name = "Bin Definitions";
            dummy.uid = proIds::NullUid;
        }
        for(auto & entry: input){
            projectSliceData recut;
            size_t i = 0, e = 0;
            //spin to first edge
            while(entry.second.slices[0].start > edges[e]) e++;
            //NOTE: guaranteed that edges.size() >= slices.size()
            for(; e<edges.size()-1; e++){
                //Adding the reduced slice
                recut.slices.push_back({edges[e], edges[e+1], entry.second.slices[i].FTE});
                //Adding to the cumulate
                if(fillCumulates) dummy.slices[e].FTE += entry.second.slices[i].FTE;
                //Moving to the next original slice when the end of this one is the next edge
                if(entry.second.slices[i].end == edges[e+1]) i++;
                //Stopping when we run out of original slices
                if(i > entry.second.slices.size()-1) break;
            }
            recut.uid = entry.second.uid;
            recut.name = entry.second.name;
            out[entry.first] = recut;
        }
        if(fillCumulates) out[dummy.uid] = dummy;
        return out;
    }
    using mapType = std::map<proIds::Uuid, std::vector<std::pair<size_t, size_t> > >;
    /**
     * @brief Reprocess onto shared times
     *
     * Takes a map of ids onto time bins and remaps each entry onto the union set of bins (i.e the set of bins that accomodates all entries). Populates a map from input slice index to output slice index for each id
     * @pre All slices have specific (not null) start and end. No slices for a single project overlap.
     * @param input Set to process.
     * @param remapping Variable to fill with the remapping, final slice id and the slice it is populated from
     * @return std::map<proIds::Uuid, projectSliceData>
     */
    static std::map<proIds::Uuid, projectSliceData> reprocessWithMap(std::map<proIds::Uuid, projectSliceData> input, mapType & remapping){
        //Forming the list of all the edges (skipping null)
        remapping.clear();
        std::vector<timecode> edges;
        for(auto & entry: input){
            for(auto & slice : entry.second.slices){
                if(slice.start != timecodeNull) edges.push_back(slice.start);
                if(slice.end != timecodeNull) edges.push_back(slice.end);
            }
        }
        //Sorting the edges
        std::sort(edges.begin(), edges.end());
        auto it = unique(edges.begin(), edges.end());
        edges.resize(std::distance(edges.begin(), it));
        // Re-cut each project onto the new edges
        // Assume we have succeeded, and can thus rely on all previous edges appearing
        std::map<proIds::Uuid, projectSliceData> out;
        for(auto & entry: input){
            projectSliceData recut;
            size_t i = 0, e = 0;
            //spin to first edge
            while(entry.second.slices[0].start > edges[e]) e++;
            //NOTE: guaranteed that edges.size() >= slices.size()
            for(; e<edges.size()-1; e++){
                //Adding the reduced slice
                recut.slices.push_back({edges[e], edges[e+1], entry.second.slices[i].FTE});
                remapping[entry.second.uid].push_back(std::make_pair(e, i));
                //Moving to the next original slice when the end of this one is the next edge
                if(entry.second.slices[i].end == edges[e+1]) i++;
                //Stopping when we run out of original slices
                if(i > entry.second.slices.size()-1) break;
            }
            recut.uid = entry.second.uid;
            recut.name = entry.second.name;
            out[entry.first] = recut;
        }
        return out;
    }
};

#endif