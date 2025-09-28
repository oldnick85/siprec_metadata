#include "siprec_metadata.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <random>
#include <ranges>
#include <string>

#include "pugixml.hpp"

using namespace siprec_metadata;

namespace siprec_metadata
{
// Generation UUID by RFC4122
std::string generate_v4()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    uint8_t uuid[16];
    for (int i = 0; i < 16; ++i) {
        uuid[i] = dis(gen);
    }

    // Set version (4) and variant (10)
    uuid[6] = (uuid[6] & 0x0F) | 0x40;
    uuid[8] = (uuid[8] & 0x3F) | 0x80;

    char uuid_str[37];
    snprintf(uuid_str, sizeof(uuid_str), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10],
             uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
    return std::string(uuid_str);
}

constexpr std::string_view base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string base64_encode(std::string_view data)
{
    std::string encoded;
    encoded.reserve(((data.size() + 2) / 3) * 4);

    auto convert_group = [](uint32_t triple, int bytes) -> std::array<char, 4> {
        uint32_t sextet = (triple << 6) >> 6;
        std::array<char, 4> quad;
        for (int i = 3; i >= 0; --i) {
            quad[i] = base64_chars[sextet & 0x3F];
            sextet >>= 6;
        }
        for (int i = bytes + 1; i < 4; ++i) quad[i] = '=';
        return quad;
    };

    size_t i = 0;
    while (i + 2 < data.size()) {
        uint32_t triple = (static_cast<uint8_t>(data[i]) << 16) | (static_cast<uint8_t>(data[i + 1]) << 8)
                          | static_cast<uint8_t>(data[i + 2]);
        auto quad = convert_group(triple, 3);
        encoded.append(quad.begin(), quad.end());
        i += 3;
    }

    if (i < data.size()) {
        uint32_t triple = 0;
        int bytes = 0;
        for (int j = 0; j < 2 && i + j < data.size(); ++j) {
            triple |= static_cast<uint8_t>(data[i + j]) << (16 - 8 * j);
            bytes = j + 1;
        }
        auto quad = convert_group(triple, bytes);
        encoded.append(quad.begin(), quad.end());
    }

    return encoded;
}

std::string generate_unique_id()
{
    const auto uuid = generate_v4();
    return base64_encode(uuid);
}

bool FromXML(std::list<Participant>& participants, const pugi::xml_node& node)
{
    auto participant_id_attr = node.attribute("participant_id");
    if (not participant_id_attr) {
        return false;
    }
    Participant participant{participant_id_attr.value()};

    for (auto name_id_node : node.children("nameID")) {
        std::string aor = name_id_node.attribute("aor").value();
        if (auto name_node = name_id_node.child("name")) {
            std::string name = name_node.text().get();
            participant.AddNameId(name, aor);
        }
    }

    participants.push_back(participant);

    return true;
}

bool FromXML(std::list<ParticipantSessionAssociation>& participant_session_associations, const pugi::xml_node& node)
{
    ParticipantSessionAssociation participant_session_association;

    participant_session_association.SetParticipant(node.attribute("participant_id").value());
    participant_session_association.SetSession(node.attribute("session_id").value());

    if (auto associate_time_node = node.child("associate-time")) {
        participant_session_association.SetAssociateTime(associate_time_node.text().get());
    }

    if (auto disassociate_time_node = node.child("disassociate-time")) {
        participant_session_association.SetDisassociateTime(disassociate_time_node.text().get());
    }

    for (auto param_node : node.children("param")) {
        participant_session_association.AddParam(param_node.text().get());
    }

    participant_session_associations.push_back(participant_session_association);

    return true;
}

bool FromXML(std::list<ParticipantStreamAssociation>& participant_stream_associations, const pugi::xml_node& node)
{
    auto participant_id_attr = node.attribute("participant_id");
    if (not participant_id_attr)
        return false;

    const std::string participant_id = participant_id_attr.value();

    for (auto send_node : node.children("send")) {
        const std::string stream_id = send_node.text().get();

        auto participant_stream_association_it = std::ranges::find_if(
            participant_stream_associations, [&](const ParticipantStreamAssociation& participant_stream_association) {
                return ((participant_stream_association.ParticipantId() == participant_id)
                        and (participant_stream_association.StreamId() == stream_id));
            });
        if (participant_stream_association_it == participant_stream_associations.end()) {
            ParticipantStreamAssociation participant_stream_association;
            participant_stream_association.SetParticipant(participant_id);
            participant_stream_association.SetStream(stream_id);
            participant_stream_association.SetSend(true);
            participant_stream_associations.push_back(participant_stream_association);
        } else {
            participant_stream_association_it->SetSend(true);
        }
    }

    for (auto recv_node : node.children("recv")) {
        const std::string stream_id = recv_node.text().get();

        auto participant_stream_association_it = std::ranges::find_if(
            participant_stream_associations, [&](const ParticipantStreamAssociation& participant_stream_association) {
                return ((participant_stream_association.ParticipantId() == participant_id)
                        and (participant_stream_association.StreamId() == stream_id));
            });
        if (participant_stream_association_it == participant_stream_associations.end()) {
            ParticipantStreamAssociation participant_stream_association;
            participant_stream_association.SetParticipant(participant_id);
            participant_stream_association.SetStream(stream_id);
            participant_stream_association.SetRecv(true);
            participant_stream_associations.push_back(participant_stream_association);
        } else {
            participant_stream_association_it->SetRecv(true);
        }
    }

    return true;
}

bool FromXML(std::list<CSRSAssociation>& csrs_associations, const pugi::xml_node& node)
{
    CSRSAssociation csrs_association;
    csrs_association.SetSession(node.attribute("session_id").value());

    if (auto associate_time_node = node.child("associate-time")) {
        csrs_association.SetAssociateTime(Timestamp::from_rfc3339(associate_time_node.text().get()));
    }

    if (auto disassociate_time_node = node.child("disassociate-time")) {
        csrs_association.SetDisassociateTime(Timestamp::from_rfc3339(disassociate_time_node.text().get()));
    }

    csrs_associations.push_back(csrs_association);

    return true;
}

bool FromXML(std::list<MediaStream>& streams, const pugi::xml_node& node)
{
    auto stream_id_attr = node.attribute("stream_id");
    if (not stream_id_attr) {
        return false;
    }
    MediaStream stream{stream_id_attr.value()};

    auto session_id_attr = node.attribute("session_id");
    if (not session_id_attr) {
        return false;
    }
    stream.SetSessionId(session_id_attr.value());

    if (auto label_node = node.child("label")) {
        stream.SetLabel(label_node.text().get());
    }

    if (auto content_type_node = node.child("content-type")) {
        stream.SetContentType(content_type_node.text().get());
    }

    streams.push_back(stream);

    return true;
}

bool FromXML(std::list<CommunicationSession>& sessions, const pugi::xml_node& node)
{
    auto session_id_attr = node.attribute("session_id");
    if (not session_id_attr) {
        return false;
    }
    CommunicationSession session{session_id_attr.value()};

    if (auto group_ref_node = node.child("group-ref")) {
        session.SetGroupRef(group_ref_node.text().get());
    }

    if (auto reason_node = node.child("reason")) {
        session.SetReason(reason_node.text().get());
    }

    if (auto start_time_node = node.child("start-time")) {
        session.SetStartTime(Timestamp::from_rfc3339(start_time_node.text().get()));
    }

    if (auto stop_time_node = node.child("stop-time")) {
        session.SetStopTime(Timestamp::from_rfc3339(stop_time_node.text().get()));
    }

    for (auto sip_session_node : node.children("sipSessionID")) {
        session.AddSipSessionId(sip_session_node.text().get());
    }

    sessions.push_back(session);

    return true;
}

bool FromXML(std::list<CommunicationSessionGroup>& groups, const pugi::xml_node& node)
{
    auto group_id_attr = node.attribute("group_id");
    if (not group_id_attr) {
        return false;
    }
    CommunicationSessionGroup group{group_id_attr.value()};

    if (auto associate_time_node = node.child("associate-time")) {
        group.SetAssociateTime(Timestamp::from_rfc3339(associate_time_node.text().get()));
    }

    if (auto disassociate_time_node = node.child("disassociate-time")) {
        group.SetDisassociateTime(Timestamp::from_rfc3339(disassociate_time_node.text().get()));
    }

    groups.push_back(group);

    return true;
}

pugi::xml_node ToXML(const Participant& participant, pugi::xml_node& parent)
{
    auto participant_node = parent.append_child("participant");
    participant_node.append_attribute("participant_id").set_value(participant.ParticipantId());

    for (const auto& [name, aor] : participant.NameIds()) {
        auto name_id_node = participant_node.append_child("nameID");
        name_id_node.append_attribute("aor").set_value(aor);
        if (!name.empty()) {
            auto name_node = name_id_node.append_child("name");
            name_node.append_attribute("xml:lang").set_value("it");
            name_node.text().set(name);
        }
    }

    return participant_node;
}

pugi::xml_node ToXML(const MediaStream& stream, pugi::xml_node& parent)
{
    auto stream_node = parent.append_child("stream");
    stream_node.append_attribute("stream_id").set_value(stream.StreamId());
    stream_node.append_attribute("session_id").set_value(stream.SessionId());

    if (not stream.Label().empty()) {
        stream_node.append_child("label").text().set(stream.Label());
    }

    if (stream.ContentType()) {
        stream_node.append_child("content-type").text().set(stream.ContentType().value());
    }

    return stream_node;
}

pugi::xml_node ToXML(const ParticipantSessionAssociation& participant_session_association, pugi::xml_node& parent)
{
    auto assoc_node = parent.append_child("participantsessionassoc");
    assoc_node.append_attribute("participant_id").set_value(participant_session_association.ParticipantId());
    assoc_node.append_attribute("session_id").set_value(participant_session_association.SessionId());

    assoc_node.append_child("associate-time").text().set(participant_session_association.AssociateTime().to_rfc3339());

    if (participant_session_association.DisassociateTime()) {
        assoc_node.append_child("disassociate-time")
            .text()
            .set(participant_session_association.DisassociateTime().value().to_rfc3339());
    }

    for (const auto& param : participant_session_association.Params()) {
        assoc_node.append_child("param").text().set(param);
    }

    return assoc_node;
}

pugi::xml_node ToXML(const CSRSAssociation& csrs_association, pugi::xml_node& parent)
{
    auto assoc_node = parent.append_child("sessionrecordingassoc");
    assoc_node.append_attribute("session_id").set_value(csrs_association.SessionId());

    assoc_node.append_child("associate-time").text().set(csrs_association.AssociateTime().to_rfc3339());

    if (csrs_association.DisassociateTime()) {
        assoc_node.append_child("disassociate-time")
            .text()
            .set(csrs_association.DisassociateTime().value().to_rfc3339());
    }

    return assoc_node;
}

pugi::xml_node ToXML(const CommunicationSession& communication_session, pugi::xml_node& parent)
{
    auto session_node = parent.append_child("session");
    session_node.append_attribute("session_id").set_value(communication_session.SessionId());

    if (communication_session.Reason()) {
        session_node.append_child("reason").text().set(communication_session.Reason().value());
    }

    if (communication_session.StartTime()) {
        session_node.append_child("start-time").text().set(communication_session.StartTime().value().to_rfc3339());
    }

    if (communication_session.StopTime()) {
        session_node.append_child("stop-time").text().set(communication_session.StopTime().value().to_rfc3339());
    }

    for (const auto& sip_session_id : communication_session.SipSessionIds()) {
        session_node.append_child("sipSessionID").text().set(sip_session_id);
    }

    if (communication_session.GroupRef()) {
        session_node.append_child("group-ref").text().set(communication_session.GroupRef().value());
    }

    return session_node;
}

pugi::xml_node ToXML(const CommunicationSessionGroup& group, pugi::xml_node& parent)
{
    auto group_node = parent.append_child("group");
    group_node.append_attribute("group_id").set_value(group.GroupId());

    if (group.AssociateTime()) {
        group_node.append_child("associate-time").text().set(group.AssociateTime().value().to_rfc3339());
    }

    if (group.DisassociateTime()) {
        group_node.append_child("disassociate-time").text().set(group.DisassociateTime().value().to_rfc3339());
    }

    return group_node;
}
}  // namespace siprec_metadata

bool Timestamp::operator==(const Timestamp& other) const { return (time_ == other.time_); }

std::string Timestamp::to_rfc3339() const
{
    auto time_t = std::chrono::system_clock::to_time_t(time_);
    std::tm tm = *std::gmtime(&time_t);

    char buffer[30];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return std::string(buffer);
}

Timestamp Timestamp::from_rfc3339(const std::string& rfc3339)
{
    std::chrono::sys_seconds tp;
    std::istringstream ss(rfc3339);

    // Парсим напрямую в системное время (всегда UTC)
    ss >> std::chrono::parse("%Y-%m-%dT%H:%M:%SZ", tp);

    if (ss.fail()) {
        return Timestamp{};
    }

    return Timestamp(tp);

    /*std::tm tm = {};
    std::istringstream ss(rfc3339);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    const auto time_t = std::mktime(&tm);
    const auto timepoint = std::chrono::system_clock::from_time_t(time_t);
    return Timestamp(timepoint);*/
}

Timestamp Timestamp::now() { return Timestamp(std::chrono::system_clock::now()); }

Participant::Participant() : participant_id_(generate_unique_id()) {}

Participant::Participant(const std::string& id) : participant_id_(id)
{
    if (participant_id_.empty())
        participant_id_ = generate_unique_id();
}

bool Participant::operator==(const Participant& other) const
{
    return ((participant_id_ == other.participant_id_) and (name_id_ == other.name_id_));
}

MediaStream::MediaStream() : stream_id_(generate_unique_id()) {}

MediaStream::MediaStream(const std::string& stream_id) : stream_id_(stream_id)
{
    if (stream_id_.empty())
        stream_id_ = generate_unique_id();
}

bool MediaStream::operator==(const MediaStream& other) const
{
    return ((stream_id_ == other.stream_id_) and (label_ == other.label_) and (content_type_ == other.content_type_)
            and (session_id_ == other.session_id_));
}

const std::string& MediaStream::StreamId() const { return stream_id_; }

const std::string& MediaStream::Label() const { return label_; }

const std::optional<std::string>& MediaStream::ContentType() const { return content_type_; }

const std::string& MediaStream::SessionId() const { return session_id_; }

void MediaStream::SetSessionId(const std::string& session_id) { session_id_ = session_id; }

void MediaStream::SetLabel(const std::string& label) { label_ = label; }

void MediaStream::SetContentType(const std::string& content_type) { content_type_ = content_type; }

bool ParticipantStreamAssociation::operator==(const ParticipantStreamAssociation& other) const
{
    return ((associate_time_ == other.associate_time_) and (disassociate_time_ == other.disassociate_time_)
            and (send_ == other.send_) and (recv_ == other.recv_) and (participant_id_ == other.participant_id_)
            and (stream_id_ == other.stream_id_));
}

const Timestamp& ParticipantStreamAssociation::AssociateTime() const { return associate_time_; }

const auto& ParticipantStreamAssociation::DisassociateTime() const { return disassociate_time_; }

bool ParticipantStreamAssociation::IsSender() const { return send_; }

bool ParticipantStreamAssociation::IsReceiver() const { return recv_; }

const std::string& ParticipantStreamAssociation::ParticipantId() const { return participant_id_; }

const std::string& ParticipantStreamAssociation::StreamId() const { return stream_id_; }

void ParticipantStreamAssociation::SetParticipant(const std::string& participant_id)
{
    participant_id_ = participant_id;
}

void ParticipantStreamAssociation::SetStream(const std::string& stream_id) { stream_id_ = stream_id; }

void ParticipantStreamAssociation::SetSend(bool send) { send_ = send; }

void ParticipantStreamAssociation::SetRecv(bool recv) { recv_ = recv; }

void ParticipantStreamAssociation::SetAssociateTime(const Timestamp& time) { associate_time_ = time; }

void ParticipantStreamAssociation::SetAssociateTime(const std::string& time_rfc3339)
{
    associate_time_ = Timestamp::from_rfc3339(time_rfc3339);
}

void ParticipantStreamAssociation::SetDisassociateTime(const Timestamp& time) { disassociate_time_ = time; }

void ParticipantStreamAssociation::SetDisassociateTime(const std::string& time_rfc3339)
{
    disassociate_time_ = Timestamp::from_rfc3339(time_rfc3339);
}

bool ParticipantSessionAssociation::operator==(const ParticipantSessionAssociation& other) const
{
    return ((associate_time_ == other.associate_time_) and (disassociate_time_ == other.disassociate_time_)
            and (params_ == other.params_) and (participant_id_ == other.participant_id_)
            and (session_id_ == other.session_id_));
}

const Timestamp& ParticipantSessionAssociation::AssociateTime() const { return associate_time_; }

const std::optional<Timestamp>& ParticipantSessionAssociation::DisassociateTime() const { return disassociate_time_; }

const std::list<std::string>& ParticipantSessionAssociation::Params() const { return params_; }

const std::string& ParticipantSessionAssociation::ParticipantId() const { return participant_id_; }

const std::string& ParticipantSessionAssociation::SessionId() const { return session_id_; }

void ParticipantSessionAssociation::SetParticipant(const std::string& participant_id)
{
    participant_id_ = participant_id;
}

void ParticipantSessionAssociation::SetSession(const std::string& session_id) { session_id_ = session_id; }

void ParticipantSessionAssociation::AddParam(const std::string& param) { params_.push_back(param); }

void ParticipantSessionAssociation::SetAssociateTime(const Timestamp& time) { associate_time_ = time; }

void ParticipantSessionAssociation::SetAssociateTime(const std::string& time_rfc3339)
{
    associate_time_ = Timestamp::from_rfc3339(time_rfc3339);
}

void ParticipantSessionAssociation::SetDisassociateTime(const Timestamp& time) { disassociate_time_ = time; }

void ParticipantSessionAssociation::SetDisassociateTime(const std::string& time_rfc3339)
{
    disassociate_time_ = Timestamp::from_rfc3339(time_rfc3339);
}

CommunicationSession::CommunicationSession() : session_id_(generate_unique_id()) {}

CommunicationSession::CommunicationSession(const std::string& id) : session_id_(id)
{
    if (session_id_.empty())
        session_id_ = generate_unique_id();
}

bool CommunicationSession::operator==(const CommunicationSession& other) const
{
    return ((session_id_ == other.session_id_) and (reason_ == other.reason_)
            and (sip_session_ids_ == other.sip_session_ids_) and (group_ref_ == other.group_ref_)
            and (start_time_ == other.start_time_) and (stop_time_ == other.stop_time_));
}

const std::string& CommunicationSession::SessionId() const { return session_id_; }

const std::optional<std::string>& CommunicationSession::Reason() const { return reason_; }

const std::list<std::string>& CommunicationSession::SipSessionIds() const { return sip_session_ids_; }

const std::optional<std::string>& CommunicationSession::GroupRef() const { return group_ref_; }

const std::optional<Timestamp>& CommunicationSession::StartTime() const { return start_time_; }

const std::optional<Timestamp>& CommunicationSession::StopTime() const { return stop_time_; }

void CommunicationSession::SetReason(const std::string& reason) { reason_ = reason; }

void CommunicationSession::AddSipSessionId(const std::string& session_id) { sip_session_ids_.push_back(session_id); }

void CommunicationSession::SetGroupRef(const std::string& group_ref) { group_ref_ = group_ref; }

void CommunicationSession::SetStartTime(const Timestamp& time) { start_time_ = time; }

void CommunicationSession::SetStopTime(const Timestamp& time) { stop_time_ = time; }

CommunicationSessionGroup::CommunicationSessionGroup() : group_id_(generate_unique_id()) {}

CommunicationSessionGroup::CommunicationSessionGroup(const std::string& id) : group_id_(id)
{
    if (group_id_.empty())
        group_id_ = generate_unique_id();
}

bool CommunicationSessionGroup::operator==(const CommunicationSessionGroup& other) const
{
    return ((group_id_ == other.group_id_) and (associate_time_ == other.associate_time_)
            and (disassociate_time_ == other.disassociate_time_));
}

const std::string& CommunicationSessionGroup::GroupId() const { return group_id_; }

const std::optional<Timestamp>& CommunicationSessionGroup::AssociateTime() const { return associate_time_; }

const std::optional<Timestamp>& CommunicationSessionGroup::DisassociateTime() const { return disassociate_time_; }

void CommunicationSessionGroup::SetAssociateTime(const Timestamp& time) { associate_time_ = time; }

void CommunicationSessionGroup::SetAssociateTime(const std::string& time_rfc3339)
{
    associate_time_ = Timestamp::from_rfc3339(time_rfc3339);
}

void CommunicationSessionGroup::SetDisassociateTime(const Timestamp& time) { disassociate_time_ = time; }

void CommunicationSessionGroup::SetDisassociateTime(const std::string& time_rfc3339)
{
    disassociate_time_ = Timestamp::from_rfc3339(time_rfc3339);
}

bool CSRSAssociation::operator==(const CSRSAssociation& other) const
{
    return ((associate_time_ == other.associate_time_) and (disassociate_time_ == other.disassociate_time_)
            and (session_id_ == other.session_id_));
}

const Timestamp& CSRSAssociation::AssociateTime() const { return associate_time_; }

const std::optional<Timestamp>& CSRSAssociation::DisassociateTime() const { return disassociate_time_; }

const std::string& CSRSAssociation::SessionId() const { return session_id_; }

void CSRSAssociation::SetSession(const CommunicationSession& session) { session_id_ = session.SessionId(); }

void CSRSAssociation::SetSession(const std::string& session_id) { session_id_ = session_id; }

void CSRSAssociation::SetAssociateTime(const std::string& time_rfc3339)
{
    associate_time_ = Timestamp::from_rfc3339(time_rfc3339);
}

void CSRSAssociation::SetAssociateTime(const Timestamp& timestamp) { associate_time_ = timestamp; }

void CSRSAssociation::SetDisassociateTime(const std::string& time_rfc3339)
{
    disassociate_time_ = Timestamp::from_rfc3339(time_rfc3339);
}

void CSRSAssociation::SetDisassociateTime(const Timestamp& timestamp) { disassociate_time_ = timestamp; }

CommunicationSessionGroup& RecordingSession::AddGroup(const std::string& group_id)
{
    groups_.emplace_back(group_id);
    return groups_.back();
}

CommunicationSession& RecordingSession::AddCommSession(const std::string& session_id)
{
    comm_sessions_.emplace_back(session_id);
    return comm_sessions_.back();
}

Participant& RecordingSession::AddParticipant(const std::string& participant_id)
{
    participants_.emplace_back(participant_id);
    return participants_.back();
}

MediaStream& RecordingSession::AddStream(const std::string& stream_id)
{
    media_streams_.emplace_back(stream_id);
    return media_streams_.back();
}

void RecordingSession::AddAssociation(CommunicationSession& session, MediaStream& stream)
{
    stream.SetSessionId(session.SessionId());
}

void RecordingSession::AddAssociation(CommunicationSessionGroup& group, CommunicationSession& comm_session)
{
    comm_session.SetGroupRef(group.GroupId());
}

CSRSAssociation& RecordingSession::AddAssociation(CommunicationSession& comm_session)
{
    CSRSAssociation csrs_association;
    csrs_association.SetSession(comm_session);
    csrs_associations_.emplace_back(csrs_association);
    return csrs_associations_.back();
}

ParticipantSessionAssociation& RecordingSession::AddAssociation(const CommunicationSession& session,
                                                                const Participant& participant)
{
    ParticipantSessionAssociation participant_session_association;
    participant_session_association.SetParticipant(participant.ParticipantId());
    participant_session_association.SetSession(session.SessionId());
    participant_session_associations_.push_back(participant_session_association);
    return participant_session_associations_.back();
}

void RecordingSession::AddAssociation(Participant& participant, const MediaStream& stream, bool send, bool recv)
{
    ParticipantStreamAssociation participant_stream_association;
    participant_stream_association.SetParticipant(participant.ParticipantId());
    participant_stream_association.SetStream(stream.StreamId());
    participant_stream_association.SetSend(send);
    participant_stream_association.SetRecv(recv);
    participant_stream_associations_.emplace_back(participant_stream_association);
}

const std::optional<Timestamp>& RecordingSession::StartTime() const { return start_time_; }

const std::optional<Timestamp>& RecordingSession::EndTime() const { return end_time_; }

const std::string& RecordingSession::DataMode() const { return data_mode_; }

const std::list<CommunicationSessionGroup>& RecordingSession::Groups() const { return groups_; }

const std::list<CommunicationSession>& RecordingSession::CommSessions() const { return comm_sessions_; }

const std::list<MediaStream>& RecordingSession::MediaStreams() const { return media_streams_; }

const std::list<CSRSAssociation>& RecordingSession::CS_RS_Associations() const { return csrs_associations_; }

const std::list<ParticipantSessionAssociation>& RecordingSession::ParticipantSessionAssociations() const
{
    return participant_session_associations_;
}

const std::list<ParticipantStreamAssociation>& RecordingSession::ParticipantStreamAssociations() const
{
    return participant_stream_associations_;
}

void RecordingSession::SetStartTime(const Timestamp& time) { start_time_ = time; }

void RecordingSession::SetEndTime(const Timestamp& time) { end_time_ = time; }

void RecordingSession::SetDataMode(const std::string& mode) { data_mode_ = mode; }

bool RecordingSession::operator==(const RecordingSession& other) const
{
    if (groups_.size() != other.groups_.size())
        return false;
    for (const auto& group : groups_) {
        if (not std::ranges::any_of(other.groups_,
                                    [&](const CommunicationSessionGroup& item) { return item == group; }))
            return false;
    }

    if (comm_sessions_.size() != other.comm_sessions_.size())
        return false;
    for (const auto& comm_session : comm_sessions_) {
        if (not std::ranges::any_of(other.comm_sessions_,
                                    [&](const CommunicationSession& item) { return item == comm_session; }))
            return false;
    }

    if (media_streams_.size() != other.media_streams_.size())
        return false;
    for (const auto& media_stream : media_streams_) {
        if (not std::ranges::any_of(other.media_streams_,
                                    [&](const MediaStream& item) { return item == media_stream; }))
            return false;
    }

    if (participants_.size() != other.participants_.size())
        return false;
    for (const auto& participant : participants_) {
        if (not std::ranges::any_of(other.participants_, [&](const Participant& item) { return item == participant; }))
            return false;
    }

    if (csrs_associations_.size() != other.csrs_associations_.size())
        return false;
    for (const auto& csrs_association : csrs_associations_) {
        if (not std::ranges::any_of(other.csrs_associations_,
                                    [&](const CSRSAssociation& item) { return item == csrs_association; }))
            return false;
    }

    if (participant_session_associations_.size() != other.participant_session_associations_.size())
        return false;
    for (const auto& participant_session_association : participant_session_associations_) {
        if (not std::ranges::any_of(
                other.participant_session_associations_,
                [&](const ParticipantSessionAssociation& item) { return item == participant_session_association; }))
            return false;
    }

    if (participant_stream_associations_.size() != other.participant_stream_associations_.size())
        return false;
    for (const auto& participant_stream_association : participant_stream_associations_) {
        if (not std::ranges::any_of(
                other.participant_stream_associations_,
                [&](const ParticipantStreamAssociation& item) { return item == participant_stream_association; }))
            return false;
    }

    return true;
}

std::string RecordingSession::ToXML() const
{
    pugi::xml_document doc;

    // XML declaration
    auto declaration = doc.append_child(pugi::node_declaration);
    declaration.append_attribute("version") = "1.0";
    declaration.append_attribute("encoding") = "UTF-8";

    auto recording_node = doc.append_child("recording");
    recording_node.append_attribute("xmlns").set_value("urn:ietf:params:xml:ns:recording:1");

    recording_node.append_child("datamode").text().set(data_mode_);

    if (start_time_) {
        recording_node.append_child("start-time").text().set(start_time_->to_rfc3339());
    }

    if (end_time_) {
        recording_node.append_child("end-time").text().set(end_time_->to_rfc3339());
    }

    for (const auto& group : groups_) {
        siprec_metadata::ToXML(group, recording_node);
    }

    for (const auto& session : comm_sessions_) {
        siprec_metadata::ToXML(session, recording_node);
    }

    for (const auto& participant : participants_) {
        siprec_metadata::ToXML(participant, recording_node);
    }

    for (const auto& stream : media_streams_) {
        siprec_metadata::ToXML(stream, recording_node);
    }

    for (const auto& assoc : csrs_associations_) {
        siprec_metadata::ToXML(assoc, recording_node);
    }

    for (const auto& assoc : participant_session_associations_) {
        siprec_metadata::ToXML(assoc, recording_node);
    }

    for (const auto& participant : participants_) {
        auto assoc_node = recording_node.append_child("participantstreamassoc");
        assoc_node.append_attribute("participant_id").set_value(participant.ParticipantId());
        for (const auto& assoc : participant_stream_associations_) {
            if (assoc.ParticipantId() == participant.ParticipantId()) {
                if (assoc.IsSender()) {
                    assoc_node.append_child("send").text().set(assoc.StreamId());
                }
                if (assoc.IsReceiver()) {
                    assoc_node.append_child("recv").text().set(assoc.StreamId());
                }
            }
        }
    }

    // Convert to string
    std::ostringstream oss;
    doc.save(oss, "  ");
    return oss.str();
}

bool RecordingSession::FromXML(const std::string& xml_content)
{
    pugi::xml_document doc;
    if (!doc.load_string(xml_content.c_str())) {
        return false;
    }

    auto recording_node = doc.child("recording");
    if (!recording_node) {
        return false;
    }

    if (auto data_mode_node = recording_node.child("datamode")) {
        SetDataMode(data_mode_node.text().get());
    }

    if (auto start_time_node = recording_node.child("start-time")) {
        SetStartTime(Timestamp::from_rfc3339(start_time_node.text().get()));
    }

    if (auto end_time_node = recording_node.child("end-time")) {
        SetEndTime(Timestamp::from_rfc3339(end_time_node.text().get()));
    }

    for (auto group_node : recording_node.children("group")) {
        if (not siprec_metadata::FromXML(groups_, group_node))
            return false;
    }

    for (auto session_node : recording_node.children("session")) {
        if (not siprec_metadata::FromXML(comm_sessions_, session_node))
            return false;
    }

    for (auto stream_node : recording_node.children("stream")) {
        if (not siprec_metadata::FromXML(media_streams_, stream_node))
            return false;
    }

    for (auto participant_node : recording_node.children("participant")) {
        if (not siprec_metadata::FromXML(participants_, participant_node))
            return false;
    }

    for (auto assoc_node : recording_node.children("sessionrecordingassoc")) {
        if (not siprec_metadata::FromXML(csrs_associations_, assoc_node))
            return false;
    }

    for (auto assoc_node : recording_node.children("participantsessionassoc")) {
        if (not siprec_metadata::FromXML(participant_session_associations_, assoc_node))
            return false;
    }

    for (auto assoc_node : recording_node.children("participantstreamassoc")) {
        if (not siprec_metadata::FromXML(participant_stream_associations_, assoc_node))
            return false;
    }

    return true;
}

bool RecordingSession::Check() const
{
    for (const auto& assoc : csrs_associations_) {
        auto session_it = std::ranges::find_if(comm_sessions_, [&](const CommunicationSession& session) {
            return (session.SessionId() == assoc.SessionId());
        });
        if (session_it == comm_sessions_.end())
            return false;
    }

    for (const auto& assoc : participant_session_associations_) {
        auto participant_it = std::ranges::find_if(participants_, [&](const Participant& participant) {
            return (participant.ParticipantId() == assoc.ParticipantId());
        });
        if (participant_it == participants_.end())
            return false;

        auto session_it = std::ranges::find_if(comm_sessions_, [&](const CommunicationSession& session) {
            return (session.SessionId() == assoc.SessionId());
        });
        if (session_it == comm_sessions_.end())
            return false;
    }

    for (const auto& assoc : participant_stream_associations_) {
        auto participant_it = std::ranges::find_if(participants_, [&](const Participant& participant) {
            return (participant.ParticipantId() == assoc.ParticipantId());
        });
        if (participant_it == participants_.end())
            return false;

        auto stream_it = std::ranges::find_if(
            media_streams_, [&](const MediaStream& stream) { return (stream.StreamId() == assoc.StreamId()); });
        if (stream_it == media_streams_.end())
            return false;
    }

    return true;
}

std::string RecordingSession::ToDOT() const
{
    std::string dot;
    dot += "digraph RecordingSession {\n";

    for (const auto& group : groups_) {
        dot += std::format("group[{}];\n", group.GroupId());
    }

    for (const auto& session : comm_sessions_) {
        dot += std::format("session[{}];\n", session.SessionId());
    }

    for (const auto& participant : participants_) {
        dot += std::format("part[{}];\n", participant.ParticipantId());
    }

    for (const auto& stream : media_streams_) {
        dot += std::format("stream[{}];\n", stream.StreamId());
    }

    dot += "}\n";
    return dot;
}

/*
        node [shape=plaintext]
        A1 -> B1
        A2 -> B2
        A3 -> B3

        A1 -> A2 [label=f]
        A2 -> A3 [label=g]
        B2 -> B3 [label="g'"]
        B1 -> B3 [label="(g o f)'" tailport=s headport=s]

        { rank=same; A1 A2 A3 }
        { rank=same; B1 B2 B3 }
}
*/