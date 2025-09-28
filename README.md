# SIPREC metadata

## Introduction

A high-level C++ library for parsing, generating, and manipulating Session Recording Protocol (SIPREC) metadata.
The logic of the library is based on (RFC7865)[https://datatracker.ietf.org/doc/rfc7865/].
This library abstracts the complexity of the underlying XML schema, allowing developers to focus on building and 
integrating session recording solutions without getting bogged down in the raw details of the protocol.

To work with XML, (pugixml)[https://pugixml.org/] is used.

## Example

```c++
#include "siprec_metadata.h"
#include <iostream>

using namespace siprec_metadata;

int main()
{
    // Create new recording session
    RecordingSession recording_session;
    recording_session.SetDataMode("complete");
    // Add new communication group
    auto& group = recording_session.AddGroup();
    group.SetAssociateTime(Timestamp::now());
    // Add new communication session
    auto& comm_session = recording_session.AddCommSession();
    comm_session.AddSipSessionId("ab30317f1a784dc48ff824d0d3715d86;remote=47755a9de7794ba387653f2099600ef2");
    // Associate communication session with a group
    recording_session.AddAssociation(group, comm_session);
    // Add new participant
    auto& participant = recording_session.AddParticipant();
    participant.AddNameId("Bob", "sip:bob@biloxi.com");
    // Add new stream
    auto& stream = recording_session.AddStream();
    stream.SetLabel("96");
    // Associate stream with a communication session
    recording_session.AddAssociation(comm_session, stream);
    // Associate communication session to recording session
    auto& cs_rs_assoc = recording_session.AddAssociation(comm_session);
    cs_rs_assoc.SetAssociateTime(Timestamp::now());
    // Assosiate participant to communication session
    auto& ps_assoc = recording_session.AddAssociation(comm_session, participant);
    ps_assoc.SetAssociateTime(Timestamp::now());
    // Associate stream to participant
    recording_session.AddAssociation(participant, stream, true, false);
    // Check recording session correctness
    if (not recording_session.Check())
        return -1;
    // Export recording session to XML
    const std::string xml = recording_session.ToXML();

    std::cout << "Generated XML:\n--------\n" << xml << std::endl << "--------" << std::endl;

    // Create a blank for parsing
    RecordingSession recording_session_new;

    // Parse XML with metadata
    if (not recording_session_new.FromXML(xml))
        return -1;

    if (not recording_session_new.Check())
        return -1;

    return 0;
}
```