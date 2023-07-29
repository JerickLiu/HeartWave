#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include <QVector>
#include <QDateTime>

class Session {
public:
    const int session_id;
    const int challenge_level;
    const double low_percentage;
    const double med_percentage;
    const double high_percentage;
    const double avg_coherence;
    const int total_time;
    const double achievement_score;
    const QVector<double> time_data;
    const QVector<double> bpm_data;
	const QDateTime date_time;
    Session(int id,
            int level,
            double low,
            double med,
            double high,
            double avg,
            int total,
            double score,
            QVector<double> &time,
            QVector<double> &bpm,
            QDateTime date_time):
        session_id(id),
        challenge_level(level),
        low_percentage(low),
        med_percentage(med),
        high_percentage(high),
        avg_coherence(avg),
        total_time(total),
        achievement_score(score),
        time_data(time),
        bpm_data(bpm),
		date_time(date_time) {} 
};

class Database {
public:
    Database();
    static const QString DATABASE_PATH;

    // attempts to save a session to the db
    bool add_session(
                    int level,
                    double low,
                    double med,
                    double high,
                    double avg,
                    int total,
                    double score,
                    QVector<double> &time,
                    QVector<double> &bpm);

    // returns a QVector of all session data
    QVector<Session*> get_all_sessions();

    // get the session data for a specific session by session_id
    Session* get_session_by_id(int session_id);

    // deletes a specific session by id
    void delete_session_by_id(int session_id);

    // deletes all sessions
    void delete_all_sessions();
private:
    QSqlDatabase db;

    // converts an SQL query to a session class
    Session* query_to_session(QSqlQuery session);

    // print function
    void print(const QString& line);
};

#endif // DATABASE_H
