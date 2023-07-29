#include "database.h"
#include <QDebug>
#include <QSqlError>

const QString Database::DATABASE_PATH = "/db/heartwave.db";

Database::Database() {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("heartwave.db");

    if (!db.open()) {
        throw "error opening database";
    }

    db.transaction();
    QSqlQuery query;

    // table to store session data
    query.exec("CREATE TABLE IF NOT EXISTS sessions ("
                   "session_id INTEGER NOT NULL UNIQUE PRIMARY KEY AUTOINCREMENT,"
                   "challenge_level INTEGER NOT NULL,"
                   "low_percentage REAL NOT NULL,"
                   "med_percentage REAL NOT NULL,"
                   "high_percentage REAL NOT NULL,"
                   "avg_coherence REAL NOT NULL,"
                   "total_time INTEGER NOT NULL,"
                   "achievement_score REAL NOT NULL,"
                   "date_time DATETIME NOT NULL"
               ");");

    // table to store heartrate data for each session
    query.exec("CREATE TABLE IF NOT EXISTS heartrate_data ("
                   "session_id INTEGER NOT NULL  REFERENCES sessions(session_id) ON DELETE CASCADE,"
                   "time REAL NOT NULL,"
                   "bpm REAL NOT NULL,"
                   "PRIMARY KEY (session_id, time)"
               ");");

    if (!db.commit()) {
        throw "error initializing database";
    }
}

bool Database::add_session(
        int challenge_level,
        double low_percentage,
        double med_percentage,
        double high_percentage,
        double avg_coherence,
        int total_time,
        double achievement_score,
        QVector<double> &time_data,
        QVector<double> &bpm_data) {
    db.transaction();
    QSqlQuery query;
    query.prepare("INSERT INTO sessions ("
                      "challenge_level,"
                      "low_percentage,"
                      "med_percentage,"
                      "high_percentage,"
                      "avg_coherence,"
                      "total_time,"
                      "achievement_score,"
                      "date_time"
                  ") VALUES ("
                      ":challenge_level,"
                      ":low_percentage,"
                      ":med_percentage,"
                      ":high_percentage,"
                      ":avg_coherence,"
                      ":total_time,"
                      ":achievement_score,"
                      ":date_time"
                  ");");

    query.bindValue(":challenge_level", challenge_level);
    query.bindValue(":low_percentage", low_percentage);
    query.bindValue(":med_percentage", med_percentage);
    query.bindValue(":high_percentage", high_percentage);
    query.bindValue(":avg_coherence", avg_coherence);
    query.bindValue(":total_time", total_time);
    query.bindValue(":achievement_score", achievement_score);
    query.bindValue(":date_time", QDateTime::currentDateTime());

    if (!query.exec()) {
        qDebug() << "Error inserting session: " << query.lastError();
        db.rollback();
        return false;
    }

    int session_id = query.lastInsertId().toInt();
    print("Inserting session number: " + QString::number(session_id));

    for (int i = 0; i < time_data.size(); i++) {
        if (i == bpm_data.size()) break;
        query.prepare("INSERT INTO heartrate_data ("
                          "session_id,"
                          "time,"
                          "bpm"
                      ") VALUES ("
                          ":session_id,"
                          ":time,"
                          ":bpm"
                      ");");
        query.bindValue(":session_id", session_id);
        query.bindValue(":time", time_data[i]);
        query.bindValue(":bpm", bpm_data[i]);

        if (!query.exec()) {
            qDebug() << "Error inserting heartrate_data: " << query.lastError();
            db.rollback();
            return false;
        }
    }
    return db.commit();
}

QVector<Session*> Database::get_all_sessions() {
    db.transaction();
    QSqlQuery session_query;
    session_query.prepare("SELECT * FROM sessions");
    session_query.exec();

    QVector<Session*> sessions;
    while (session_query.next()) {
        sessions.push_back(query_to_session(session_query));
    }
    return sessions;
}

Session* Database::get_session_by_id(int session_id) {
    db.transaction();
    QSqlQuery query;
    query.prepare("SELECT * FROM sessions WHERE session_id=:session_id");
    query.bindValue(":session_id", session_id);
    query.exec();

    if (query.next()) {
        return query_to_session(query);
    }
    return nullptr;
}

Session* Database::query_to_session(QSqlQuery session) {
    int session_id = session.value(0).toInt();
    int challenge_level = session.value(1).toInt();
    double low_percentage = session.value(2).toDouble();
    double med_percentage = session.value(3).toDouble();
    double high_percentage = session.value(4).toDouble();
    double avg_coherence = session.value(5).toDouble();
    int total_time = session.value(6).toInt();
    double achievement_score = session.value(7).toDouble();
	QDateTime date_time = session.value(8).toDateTime();

    db.transaction();
    QSqlQuery query;
    query.prepare("SELECT * FROM heartrate_data WHERE session_id=:session_id");
    query.bindValue(":session_id", session_id);
    query.exec();

    QVector<double> time_data;
    QVector<double> bpm_data;
    while (query.next()) {
        time_data.push_back(query.value(1).toDouble());
        bpm_data.push_back(query.value(2).toDouble());
    }
    return new Session(session_id, challenge_level, low_percentage, med_percentage, high_percentage, avg_coherence, total_time, achievement_score, time_data, bpm_data, date_time);
}


void Database::delete_session_by_id(int session_id) {
    db.transaction();
    QSqlQuery query;
    query.prepare("DELETE FROM sessions WHERE session_id=:session_id");
    query.bindValue(":session_id", session_id);
    query.exec();

    if (!db.commit()) {
        throw "error deleting session: " + QString::number(session_id);
    }
}

void Database::delete_all_sessions() {
    print("Deleting sessions");
    db.transaction();
    QSqlQuery query;
    query.exec("DELETE FROM sessions");

    if (!db.commit()) {
        throw "error deleting sessions";
    }
}

void Database::print(const QString &line) {
    qInfo("%s", line.toStdString().c_str()); // prints line to terminal
}
