#ifndef MYTAG_H
#define MYTAG_H

#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include <iostream>

using namespace ns3;

class MyTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  // these are our accessors to our tag structure
  /**
   * Set the tag value
   * \param value The tag value.
   */
  void SetSimpleValue (uint32_t value);
  /**
   * Get the tag value
   * \return the tag value.
   */
  uint32_t GetSimpleValue (void) const;

private:
  uint32_t m_simpleValue; //!< tag value
};

TypeId
MyTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MyTag")
                          .SetParent<Tag> ()
                          .AddConstructor<MyTag> ()
                          .AddAttribute ("SimpleValue", "A simple value", EmptyAttributeValue (),
                                         MakeUintegerAccessor (&MyTag::GetSimpleValue),
                                         MakeUintegerChecker<uint32_t> ());
  return tid;
}
TypeId
MyTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t
MyTag::GetSerializedSize (void) const
{
  return 4;
}
void
MyTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_simpleValue);
}
void
MyTag::Deserialize (TagBuffer i)
{
  m_simpleValue = i.ReadU32 ();
}
void
MyTag::Print (std::ostream &os) const
{
  os << "v=" << (uint32_t) m_simpleValue;
}
void
MyTag::SetSimpleValue (uint32_t value)
{
  m_simpleValue = value;
}
uint32_t
MyTag::GetSimpleValue (void) const
{
  return m_simpleValue;
}

#endif