// siprec_metadata.h
#pragma once

#include <chrono>
#include <list>
#include <string>

/**
 * @brief Session Initiation Protocol (SIP) Recording Metadata
 *
 * https://datatracker.ietf.org/doc/rfc7865/
 *
 */
namespace siprec_metadata
{

/**
 * @brief Timestamps in RFC3339 format
 *
 */
class Timestamp
{
   private:
    std::chrono::system_clock::time_point time_;

   public:
    Timestamp() = default;
    explicit Timestamp(std::chrono::system_clock::time_point tp) : time_(tp) {}

    bool operator==(const Timestamp &other) const;

    std::string to_rfc3339() const;

    static Timestamp from_rfc3339(const std::string &rfc3339);

    static Timestamp now();
};

/**
 * @brief Participant
 *
 */
class Participant
{
   private:
    std::string participant_id_;
    std::list<std::pair<std::string, std::string>> name_id_;  // (Name, AoR)

   public:
    Participant();
    explicit Participant(const std::string &id);

    bool operator==(const Participant &other) const;

    const std::string &ParticipantId() const { return participant_id_; }
    const auto &NameIds() const { return name_id_; }

    void AddNameId(const std::string &name, const std::string &aor) { name_id_.emplace_back(name, aor); }
};

/**
 * @brief Media stream
 *
 */
class MediaStream
{
   private:
    std::string stream_id_;
    std::string label_;
    std::optional<std::string> content_type_;
    std::string session_id_;

   public:
    MediaStream();
    MediaStream(const std::string &stream_id);

    bool operator==(const MediaStream &other) const;

    const std::string &StreamId() const;
    const std::string &Label() const;
    const std::optional<std::string> &ContentType() const;
    const std::string &SessionId() const;

    void SetSessionId(const std::string &session_id);
    void SetLabel(const std::string &label);
    void SetContentType(const std::string &content_type);
};

/**
 * @brief Participant stream association
 *
 */
class ParticipantStreamAssociation
{
   private:
    Timestamp associate_time_;
    std::optional<Timestamp> disassociate_time_;
    bool send_ = false;
    bool recv_ = false;
    std::string participant_id_;
    std::string stream_id_;

   public:
    ParticipantStreamAssociation() = default;

    bool operator==(const ParticipantStreamAssociation &other) const;

    const Timestamp &AssociateTime() const;
    const auto &DisassociateTime() const;
    bool IsSender() const;
    bool IsReceiver() const;
    const std::string &ParticipantId() const;
    const std::string &StreamId() const;

    void SetParticipant(const std::string &participant_id);
    void SetStream(const std::string &stream_id);
    void SetSend(bool send);
    void SetRecv(bool recv);
    void SetAssociateTime(const Timestamp &time);
    void SetAssociateTime(const std::string &time_rfc3339);
    void SetDisassociateTime(const Timestamp &time);
    void SetDisassociateTime(const std::string &time_rfc3339);
};

/**
 * @brief ParticipantSessionAssociation
 *
 */
class ParticipantSessionAssociation
{
   private:
    Timestamp associate_time_;
    std::optional<Timestamp> disassociate_time_;
    std::list<std::string> params_;
    std::string participant_id_;
    std::string session_id_;

   public:
    ParticipantSessionAssociation() = default;

    bool operator==(const ParticipantSessionAssociation &other) const;

    const Timestamp &AssociateTime() const;
    const std::optional<Timestamp> &DisassociateTime() const;
    const std::list<std::string> &Params() const;
    const std::string &ParticipantId() const;
    const std::string &SessionId() const;

    void SetParticipant(const std::string &participant_id);
    void SetSession(const std::string &session_id);
    void AddParam(const std::string &param);
    void SetAssociateTime(const Timestamp &time);
    void SetAssociateTime(const std::string &time_rfc3339);
    void SetDisassociateTime(const Timestamp &time);
    void SetDisassociateTime(const std::string &time_rfc3339);
};

/**
 * @brief Communication Session
 *
 */
class CommunicationSession
{
   private:
    std::string session_id_;
    std::optional<std::string> reason_;
    std::list<std::string> sip_session_ids_;
    std::optional<std::string> group_ref_;
    std::optional<Timestamp> start_time_;
    std::optional<Timestamp> stop_time_;

   public:
    CommunicationSession();
    explicit CommunicationSession(const std::string &id);

    bool operator==(const CommunicationSession &other) const;

    const std::string &SessionId() const;
    const std::optional<std::string> &Reason() const;
    const std::list<std::string> &SipSessionIds() const;
    const std::optional<std::string> &GroupRef() const;
    const std::optional<Timestamp> &StartTime() const;
    const std::optional<Timestamp> &StopTime() const;

    void SetReason(const std::string &reason);
    void AddSipSessionId(const std::string &session_id);
    void SetGroupRef(const std::string &group_ref);
    void SetStartTime(const Timestamp &time);
    void SetStopTime(const Timestamp &time);
};

/**
 * @brief Communication sessions group
 *
 */
class CommunicationSessionGroup
{
   private:
    std::string group_id_;
    std::optional<Timestamp> associate_time_;
    std::optional<Timestamp> disassociate_time_;

   public:
    CommunicationSessionGroup();
    explicit CommunicationSessionGroup(const std::string &id);

    bool operator==(const CommunicationSessionGroup &other) const;

    const std::string &GroupId() const;
    const std::optional<Timestamp> &AssociateTime() const;
    const std::optional<Timestamp> &DisassociateTime() const;

    void SetAssociateTime(const Timestamp &time);
    void SetAssociateTime(const std::string &time_rfc3339);
    void SetDisassociateTime(const Timestamp &time);
    void SetDisassociateTime(const std::string &time_rfc3339);
};

/**
 * @brief Assosiation Communication Session - Recording Session
 *
 */
class CSRSAssociation
{
   private:
    Timestamp associate_time_;
    std::optional<Timestamp> disassociate_time_;
    std::string session_id_;

   public:
    CSRSAssociation() = default;

    bool operator==(const CSRSAssociation &other) const;

    const Timestamp &AssociateTime() const;
    const std::optional<Timestamp> &DisassociateTime() const;
    const std::string &SessionId() const;

    void SetSession(const CommunicationSession &session);
    void SetSession(const std::string &session_id);
    void SetAssociateTime(const std::string &time_rfc3339);
    void SetAssociateTime(const Timestamp &timestamp);
    void SetDisassociateTime(const std::string &time_rfc3339);
    void SetDisassociateTime(const Timestamp &timestamp);
};

/**
 * @brief RecordingSession
 *
 */
class RecordingSession
{
   private:
    std::optional<Timestamp> start_time_;
    std::optional<Timestamp> end_time_;
    std::string data_mode_ = "complete";

    std::list<CommunicationSessionGroup> groups_;
    std::list<CommunicationSession> comm_sessions_;
    std::list<MediaStream> media_streams_;
    std::list<Participant> participants_;
    std::list<CSRSAssociation> csrs_associations_;
    std::list<ParticipantSessionAssociation> participant_session_associations_;
    std::list<ParticipantStreamAssociation> participant_stream_associations_;

   public:
    bool operator==(const RecordingSession &other) const;

    bool Check() const;

    const std::optional<Timestamp> &StartTime() const;
    const std::optional<Timestamp> &EndTime() const;
    const std::string &DataMode() const;
    const std::list<CommunicationSessionGroup> &Groups() const;
    const std::list<CommunicationSession> &CommSessions() const;
    const std::list<MediaStream> &MediaStreams() const;
    const std::list<CSRSAssociation> &CS_RS_Associations() const;
    const std::list<ParticipantSessionAssociation> &ParticipantSessionAssociations() const;
    const std::list<ParticipantStreamAssociation> &ParticipantStreamAssociations() const;

    void SetStartTime(const Timestamp &time);
    void SetEndTime(const Timestamp &time);
    void SetDataMode(const std::string &mode);

    CommunicationSessionGroup &AddGroup(const std::string &group_id = "");

    CommunicationSession &AddCommSession(const std::string &session_id = "");

    Participant &AddParticipant(const std::string &participant_id = "");

    MediaStream &AddStream(const std::string &stream_id = "");

    void AddAssociation(CommunicationSession &session, MediaStream &stream);

    void AddAssociation(CommunicationSessionGroup &group, CommunicationSession &comm_session);

    CSRSAssociation &AddAssociation(CommunicationSession &comm_session);

    ParticipantSessionAssociation &AddAssociation(const CommunicationSession &session, const Participant &participant);

    void AddAssociation(Participant &participant, const MediaStream &stream, bool send, bool recv);

    std::string ToXML() const;

    bool FromXML(const std::string &xml_content);

    std::string ToDOT() const;
};

}  // namespace siprec_metadata
