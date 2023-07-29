#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTimer>
#include <QFile>
#include <QVector>
#include <QString>
#include <QDateTime>
#include "menu.h"
#include "qcustomplot.h"
#include "database.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    double coherence;
    int challenge_level;
    float medium_threshold;
    float high_threshold;
    QTimer* timer;
    bool slider_moving_right;
    int breath_pace;
    float achievement_score;
    int coherence_timer;
    int framerate;
    bool hr_contact;
    QString bpm_file;
    QString coherence_file;

    bool summary_visible;
    char summary_page;

    Menu* main_menu;
    Menu* session_view;
    Menu* delete_session_view;
    Menu* current_menu;
    QListWidget* active_qlist;
    QCustomPlot* plot;
    QVector<double> time;
    QVector<double> heart_rate;
    QVector<double> coherences;
    int bpm_file_pos;
    int coherence_file_pos;
    double initial_range;
    QVector<Session*> sessions;
    Database db;
    bool power_on;
    int battery_level;
    enum file_names { low, med, high, mixed, short_test, hr_contact_case, num };

    // initialize menus and sub menus
    void initialize_menus(Menu* main);

    // set the current menu on change
    void change_menu(const QString menuName, const QStringList items);

    // change the coherence color/text based on the difficulty and coherence numerical value
    void update_coherence();

    // setter for coherence
    void set_coherence(double ch);

    // sets the challenge level to the passed in int (should be between 1-4)
    void set_challenge_level(int level);

    // sets the breath pacer slider interval to the passed in int (should be between 1-30)
    void set_breath_pacer_interval(int interval);

    // updates the breath pacer slider ui element
    void update_breath_slider();

    // resets the slider position to 0
    void reset_breath_slider();

    // print function
    void print(const QString& line);

    // sets the power state to the passed in bool
    void set_power_state(bool on);

    // changes power state, then sets the buttons to be enabled or disabled depending on power state
    void change_power();

    // initializes slider timer and connects it to the proper slot
    void initialize_timer();

    // generates the HRV graph
    void create_hrv_graph();

    // reads BPM data and updates the HRV graph
    void update_hrv_graph();

    // resets variables used for reading coherence file
    void reset_coherence();

    // clears coherence array and calls reset_coherence()
    void stop_coherence();

    // read from the coherence txt file and update the coherence value
    void update_coherence_level();

    // resets BPM file reading variables and clears time/heart_rate vectors
    void end_hrv_graph();

    // starts a new HRV session
    void start_session();

    // stops the current HRV session and attempts to save session data to DB,
    // returns true if session saved to DB, false if not
    bool stop_session();

    // resets all variables after a session ends
    void reset_values_after_session();

    // updates the session labels (achievement, coherence, length)
    void update_labels();

    // factory reset, resets settings to default and also clears all saved sessions
    void reset_device();

    // changes the state of hr_contact and the status icon
    void change_hr_contact();

    // caluclates percent of time spent in each coherence range at end of session
    QVector<double> calculate_coherence_percentages();

    // calculates the average coherence at the end of a session
    double calculate_mean_coherence();

    // shows the menu view and hides everything else
    void show_menu();

    // shows the summary view and hides everything else
    void show_summary();

    // hides the summary view
    void hide_summary();

    // update the values shown in the summary view
    void update_summary(double, double, double, double, float, int);

    // generates the graph for summary view screen two
    void create_summary_graph(QVector<double>, QVector<double>);

    void set_input(file_names index);

    int string_to_id(QString item_name);

private slots:
    void up_button();
    void down_button();
    void left_button();
    void right_button();
    void back_button();
    void menu_button();
    void power_button();
    void ok_button();
    void charge_button();
    void update_battery_level(int value);
    void update_timer();
};
#endif // MAINWINDOW_H
