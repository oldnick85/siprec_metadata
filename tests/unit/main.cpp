#include "gtest/gtest.h"
#include "siprec_metadata.h"

using namespace siprec_metadata;

std::string base_xml_etalon = R"x(<?xml version="1.0" encoding="UTF-8"?>
<recording xmlns="urn:ietf:params:xml:ns:recording:1">
  <datamode>complete</datamode>
  <group group_id="7+OTCyoxTmqmqyA/1weDAg==">
    <associate-time>2010-12-16T23:41:07Z</associate-time>
  </group>
  <session session_id="hVpd7YQgRW2nD22h7q60JQ==">
    <sipSessionID>ab30317f1a784dc48ff824d0d3715d86;remote=47755a9de7794ba387653f2099600ef2</sipSessionID>
    <group-ref>7+OTCyoxTmqmqyA/1weDAg==</group-ref>
  </session>
  <participant participant_id="srfBElmCRp2QB23b7Mpk0w==">
    <nameID aor="sip:bob@biloxi.com">
      <name xml:lang="it">Bob</name>
    </nameID>
  </participant>
  <participant participant_id="zSfPoSvdSDCmU3A3TRDxAw==">
    <nameID aor="sip:Paul@biloxi.com">
      <name xml:lang="it">Paul</name>
    </nameID>
  </participant>
  <stream stream_id="UAAMm5GRQKSCMVvLyl4rFw==" session_id="hVpd7YQgRW2nD22h7q60JQ==">
    <label>96</label>
  </stream>
  <stream stream_id="i1Pz3to5hGk8fuXl+PbwCw==" session_id="hVpd7YQgRW2nD22h7q60JQ==">
    <label>97</label>
  </stream>
  <stream stream_id="8zc6e0lYTlWIINA6GR+3ag==" session_id="hVpd7YQgRW2nD22h7q60JQ==">
    <label>98</label>
  </stream>
  <stream stream_id="EiXGlc+4TruqqoDaNE76ag==" session_id="hVpd7YQgRW2nD22h7q60JQ==">
    <label>99</label>
  </stream>
  <sessionrecordingassoc session_id="hVpd7YQgRW2nD22h7q60JQ==">
    <associate-time>2010-12-16T23:41:07Z</associate-time>
  </sessionrecordingassoc>
  <participantsessionassoc participant_id="srfBElmCRp2QB23b7Mpk0w==" session_id="hVpd7YQgRW2nD22h7q60JQ==">
    <associate-time>2010-12-16T23:41:07Z</associate-time>
  </participantsessionassoc>
  <participantsessionassoc participant_id="zSfPoSvdSDCmU3A3TRDxAw==" session_id="hVpd7YQgRW2nD22h7q60JQ==">
    <associate-time>2010-12-16T23:41:07Z</associate-time>
  </participantsessionassoc>
  <participantstreamassoc participant_id="srfBElmCRp2QB23b7Mpk0w==">
    <send>UAAMm5GRQKSCMVvLyl4rFw==</send>
    <send>i1Pz3to5hGk8fuXl+PbwCw==</send>
    <recv>8zc6e0lYTlWIINA6GR+3ag==</recv>
    <recv>EiXGlc+4TruqqoDaNE76ag==</recv>
  </participantstreamassoc>
  <participantstreamassoc participant_id="zSfPoSvdSDCmU3A3TRDxAw==">
    <recv>UAAMm5GRQKSCMVvLyl4rFw==</recv>
    <recv>i1Pz3to5hGk8fuXl+PbwCw==</recv>
    <send>8zc6e0lYTlWIINA6GR+3ag==</send>
    <send>EiXGlc+4TruqqoDaNE76ag==</send>
  </participantstreamassoc>
</recording>
)x";

TEST(SiprecMetadata, Base)
{
    RecordingSession recording_session;
    recording_session.SetDataMode("complete");

    auto& group = recording_session.AddGroup("7+OTCyoxTmqmqyA/1weDAg==");
    group.SetAssociateTime("2010-12-16T23:41:07Z");

    auto& comm_session = recording_session.AddCommSession("hVpd7YQgRW2nD22h7q60JQ==");
    comm_session.AddSipSessionId("ab30317f1a784dc48ff824d0d3715d86;remote=47755a9de7794ba387653f2099600ef2");

    recording_session.AddAssociation(group, comm_session);

    auto& participant1 = recording_session.AddParticipant("srfBElmCRp2QB23b7Mpk0w==");
    participant1.AddNameId("Bob", "sip:bob@biloxi.com");

    auto& participant2 = recording_session.AddParticipant("zSfPoSvdSDCmU3A3TRDxAw==");
    participant2.AddNameId("Paul", "sip:Paul@biloxi.com");

    auto& stream1 = recording_session.AddStream("UAAMm5GRQKSCMVvLyl4rFw==");
    stream1.SetLabel("96");
    recording_session.AddAssociation(comm_session, stream1);

    auto& stream2 = recording_session.AddStream("i1Pz3to5hGk8fuXl+PbwCw==");
    stream2.SetLabel("97");
    recording_session.AddAssociation(comm_session, stream2);

    auto& stream3 = recording_session.AddStream("8zc6e0lYTlWIINA6GR+3ag==");
    stream3.SetLabel("98");
    recording_session.AddAssociation(comm_session, stream3);

    auto& stream4 = recording_session.AddStream("EiXGlc+4TruqqoDaNE76ag==");
    stream4.SetLabel("99");
    recording_session.AddAssociation(comm_session, stream4);

    auto& cs_rs_assoc = recording_session.AddAssociation(comm_session);
    cs_rs_assoc.SetAssociateTime("2010-12-16T23:41:07Z");

    auto& ps_assoc1 = recording_session.AddAssociation(comm_session, participant1);
    ps_assoc1.SetAssociateTime("2010-12-16T23:41:07Z");

    auto& ps_assoc2 = recording_session.AddAssociation(comm_session, participant2);
    ps_assoc2.SetAssociateTime("2010-12-16T23:41:07Z");

    recording_session.AddAssociation(participant1, stream1, true, false);
    recording_session.AddAssociation(participant1, stream2, true, false);
    recording_session.AddAssociation(participant1, stream3, false, true);
    recording_session.AddAssociation(participant1, stream4, false, true);

    recording_session.AddAssociation(participant2, stream1, false, true);
    recording_session.AddAssociation(participant2, stream2, false, true);
    recording_session.AddAssociation(participant2, stream3, true, false);
    recording_session.AddAssociation(participant2, stream4, true, false);

    ASSERT_TRUE(recording_session.Check());

    std::string xml = recording_session.ToXML();
    // std::cout << "Generated XML:\n--------\n" << xml << std::endl << "--------" << std::endl;

    ASSERT_EQ(xml, base_xml_etalon);

    RecordingSession recording_session_new;
    ASSERT_TRUE(recording_session_new.FromXML(xml));
    ASSERT_TRUE(recording_session_new.Check());
    // std::cout << "Parsed XML:\n--------\n" << recording_session_new.ToDOT() << std::endl << "--------" << std::endl;

    ASSERT_EQ(recording_session, recording_session_new);
}
