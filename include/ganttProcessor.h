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
     * @param input Set to process
     * @return std::map<proIds::Uuid, projectSliceData>
     */
    static std::map<proIds::Uuid, projectSliceData> reprocess(std::map<proIds::Uuid, projectSliceData> input){
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
        for(auto & entry: input){
            projectSliceData recut;
            size_t i = 0, e = 0;
            //spin to first edge
            while(entry.second.slices[0].start > edges[e]) e++;
            //NOTE: guaranteed that edges.size() >= slices.size()
            for(; e<edges.size()-1; e++){
                //Adding the reduced slice
                recut.slices.push_back({edges[e], edges[e+1], entry.second.slices[i].FTE});
                //Moving to the next original slice when the end of this one is the next edge
                if(entry.second.slices[i].end == edges[e+1]) i++;
                //Stopping when we run out of original slices
                if(i > entry.second.slices.size()-1) break;
            }
            out[entry.first] = recut;
        }
        return out;
    }
};

#endif