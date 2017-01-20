#pragma once

#include "routing/index_graph_starter.hpp"
#include "routing/road_graph.hpp"
#include "routing/segment.hpp"

#include "indexer/index.hpp"

#include "std/map.hpp"
#include "std/vector.hpp"

namespace routing
{
class IndexRoadGraph : public RoadGraphBase
{
public:
  IndexRoadGraph(MwmSet::MwmId const & mwmId, Index const & index, double maxSpeedKMPH,
                 IndexGraphStarter & starter, vector<Segment> const & segments,
                 vector<Junction> const & junctions);

  // IRoadGraphBase overrides:
  virtual void GetOutgoingEdges(Junction const & junction, TEdgeVector & edges) const override;
  virtual void GetIngoingEdges(Junction const & junction, TEdgeVector & edges) const override;
  virtual double GetMaxSpeedKMPH() const override;
  virtual void GetEdgeTypes(Edge const & edge, feature::TypesHolder & types) const override;
  virtual void GetJunctionTypes(Junction const & junction,
                                feature::TypesHolder & types) const override;

private:
  void GetEdges(Junction const & junction, bool isOutgoing, TEdgeVector & edges) const;
  Junction GetJunction(Segment const & segment, bool front) const;
  const Segment & GetSegment(Junction const & junction, bool isOutgoing) const;

  MwmSet::MwmId const & m_mwmId;
  Index const & m_index;
  double const m_maxSpeedKMPH;
  IndexGraphStarter & m_starter;
  map<Junction, Segment> m_beginToSegment;
  map<Junction, Segment> m_endToSegment;
};
}  // namespace routing