#include "ndn-interpacket-strain-estimator.hpp"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE("ndn.interpacketStrainEstimator");

namespace ns3 {
namespace ndn {

//---------------------------------------------------------------------------------
// A modified version of Mean-Deviation Estimator optimized for NDN packet delivery

NS_OBJECT_ENSURE_REGISTERED(InterpacketStrainEstimator);

TypeId
InterpacketStrainEstimator::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::InterpacketStrainEstimator")
      .SetParent<RttEstimator>()
      .AddConstructor<InterpacketStrainEstimator>();
      // .AddAttribute("Gain", "Gain used in estimating the RTT (smoothed RTT), must be 0 < Gain < 1",
      //               DoubleValue(0.125), MakeDoubleAccessor(&InterpacketStrainEstimator::m_gain),
      //               MakeDoubleChecker<double>())
      // .AddAttribute("Gain2", "Gain2 used in estimating the RTT (variance), must be 0 < Gain2 < 1",
      //               DoubleValue(0.25), MakeDoubleAccessor(&InterpacketStrainEstimator::m_gain2),
      //               MakeDoubleChecker<double>());
  return tid;
}

InterpacketStrainEstimator::InterpacketStrainEstimator()
{
  NS_LOG_FUNCTION(this);
}

// InterpacketStrainEstimator::InterpacketStrainEstimator(const InterpacketStrainEstimator& c)
//   : RttEstimator(c)
//   , m_gain(c.m_gain)
//   , m_gain2(c.m_gain2)
//   , m_variance(c.m_variance)
// {
//   NS_LOG_FUNCTION(this);
// }

TypeId
InterpacketStrainEstimator::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

void
InterpacketStrainEstimator::Measurement(Time m)
{
  NS_LOG_FUNCTION(this << m);
  if (m_nSamples) { // Not first
    Time err(m - m_currentEstimatedRtt);
    double gErr = err.ToDouble(Time::S) * m_gain;
    m_currentEstimatedRtt += Time::FromDouble(gErr, Time::S);
    Time difference = Abs(err) - m_variance;
    NS_LOG_DEBUG(
      "m_variance += " << Time::FromDouble(difference.ToDouble(Time::S) * m_gain2, Time::S));
    m_variance += Time::FromDouble(difference.ToDouble(Time::S) * m_gain2, Time::S);
  }
  else {                       // First sample
    m_currentEstimatedRtt = m; // Set estimate to current
    // variance = sample / 2;               // And variance to current / 2
    // m_variance = m; // try this  why????
    m_variance = Seconds(m.ToDouble(Time::S) / 2);
    NS_LOG_DEBUG("(first sample) m_variance += " << m);
  }
  m_nSamples++;
}


Ptr<RttEstimator>
InterpacketStrainEstimator::Copy() const
{
  NS_LOG_FUNCTION(this);
  return CopyObject<InterpacketStrainEstimator>(this);
}

void
InterpacketStrainEstimator::Reset()
{
  NS_LOG_FUNCTION(this);
  // Reset to initial state
  m_variance = Seconds(0);
  RttEstimator::Reset();
}

void
InterpacketStrainEstimator::SentSeq(SequenceNumber32 seq, uint32_t size)
{
  NS_LOG_FUNCTION(this << seq << size);

  RttHistory_t::iterator i;
  for (i = m_history.begin(); i != m_history.end(); ++i) {
    if (seq == i->seq) { // Found it
      i->retx = true;
      break;
    }
  }

  // Note that a particular sequence has been sent
  if (i == m_history.end())
    m_history.push_back(RttHistory(seq, size, Simulator::Now()));
}

Time
InterpacketStrainEstimator::AckSeq(SequenceNumber32 ackSeq)
{
  NS_LOG_FUNCTION(this << ackSeq);
  // An ack has been received, calculate rtt and log this measurement
  // Note we use a linear search (O(n)) for this since for the common
  // case the ack'ed packet will be at the head of the list
  Time m = Seconds(0.0);
  if (m_history.size() == 0)
    return (m); // No pending history, just exit

  for (RttHistory_t::iterator i = m_history.begin(); i != m_history.end(); ++i) {
    if (ackSeq == i->seq) { // Found it
      if (!i->retx) {
        m = Simulator::Now() - i->time; // Elapsed time
        Measurement(m);                 // Log the measurement
        ResetMultiplier();              // Reset multiplier on valid measurement
      }
      m_history.erase(i);
      break;
    }
  }

  return m;
}

} // namespace ndn
} // namespace ns3