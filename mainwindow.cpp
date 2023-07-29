#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);

    sessions = db.get_all_sessions();

    // initialize menus
    main_menu = new Menu("MAIN", nullptr);
    current_menu = main_menu;
    initialize_menus(main_menu);

    active_qlist = ui->MenuListWidget;
    active_qlist->addItems(main_menu->get_items());
    active_qlist->setCurrentRow(0);
    ui->MenuLabel->setText(main_menu->get_name());

    plot = ui->hrv_graph;

    // initally, only menu stuff should be visible
    ui->achievement_label->setVisible(false);
    ui->length_label->setVisible(false);
    ui->coherence_label->setVisible(false);
    ui->hrv_graph->setVisible(false);
    ui->coherence_range_color->setVisible(false);
    hide_summary();

    // ***SET THE INPUT TEXT FILES HERE***
    // VALID CHOICES: low, med, high, mixed, short_test, hr_contact_case
    set_input(high);

    connect(ui->up_button, SIGNAL(pressed()), this, SLOT (up_button()));
    connect(ui->down_button, SIGNAL(pressed()), this, SLOT (down_button()));
    connect(ui->left_button, SIGNAL(pressed()), this, SLOT (left_button()));
    connect(ui->right_button, SIGNAL(pressed()), this, SLOT (right_button()));
    connect(ui->ok_button, SIGNAL(pressed()), this, SLOT (ok_button()));
    connect(ui->back_button, SIGNAL(pressed()), this, SLOT (back_button()));
    connect(ui->menu_button, SIGNAL(pressed()), this, SLOT (menu_button()));
    connect(ui->power_button, SIGNAL(pressed()), this, SLOT (power_button()));
    connect(ui->charge_button, SIGNAL(pressed()), this, SLOT (charge_button()));
    connect(ui->batterylevel, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::update_battery_level);

    set_coherence(0.00);
    set_challenge_level(1);
    power_on = true;
    battery_level = 100;
    power_button();
    update_battery_level(battery_level);

    achievement_score = 0;
    coherence_timer = 0;
    set_breath_pacer_interval(10);
    framerate = 30;
    hr_contact = false;
}

MainWindow::~MainWindow() {
    delete ui;
    delete main_menu;
    delete timer;
}

void MainWindow::up_button() {
    int idx = active_qlist->currentRow() - 1;

    // if first item, go to last item
    if (idx < 0) {
        idx = active_qlist->count() - 1;
    }

    active_qlist->setCurrentRow(idx);
}

void MainWindow::down_button() {
    int idx = active_qlist->currentRow() + 1;

    if (idx > active_qlist->count() - 1) {
        idx = 0;
    }

    active_qlist->setCurrentRow(idx);
}

void MainWindow::left_button() {
    if (summary_visible) {
        if (summary_page == 1) {
            summary_page = 0;
            show_summary();
        }
    }
}

void MainWindow::right_button() {
    if (summary_visible) {
        if (summary_page == 0) {
            summary_page = 1;
            show_summary();
        }
    }
}

void MainWindow::ok_button() {
    int index = active_qlist->currentRow();
    if (index < 0) {
        // If we are currently in a session, we should exit the session, back_button()
        // does this already
        if (current_menu->get_name() == "NEW SESSION") {
            back_button();
            return;
        }
        return;
    }

    // If we start a new session
    if (current_menu->get_name() == "MAIN" && current_menu->get_item(index)->get_name() == "NEW SESSION") {
        ui->MenuListWidget->setVisible(false);
        ui->breath_pacer->setVisible(true);
        ui->achievement_label->setVisible(true);
        ui->length_label->setVisible(true);
        ui->coherence_label->setVisible(true);
        ui->hrv_graph->setVisible(true);
        ui->coherence_range_color->setVisible(true);
        current_menu = current_menu->get_item(index);
        change_menu("NEW SESSION", {});
        start_session();
        return;
    }

    // If we are in the difficulty settings, set the difficulty level based on idx + 1
    if (current_menu->get_name() == "LEVEL") {
        print("Setting challenge level to: " + QString::number(index + 1));
        set_challenge_level(index + 1);
        return;
    }

    // If we are in breath pacer settings, update the interval
    if (current_menu->get_name() == "BREATH PACER") {
        print("Setting breath interval to: " + QString::number(index + 1));
        set_breath_pacer_interval(index + 1);
        update_breath_slider();
        return;
    }

    // If we are in delete, delete the session of the currently selected ID
    if (current_menu->get_name() == "DELETE") {
        try {
            int session_id = string_to_id(current_menu->get_item(index)->get_name());
            if (session_id == -1) {
                print("Error: invalid index");
            }
            db.delete_session_by_id(session_id);

            QString session_name = current_menu->get_item(index)->get_name();

            // remove session from both view and delete menus
            current_menu->remove_sub_menu(session_name);
            session_view->remove_sub_menu(session_name);

            // update sessions array
            sessions = db.get_all_sessions();

            // update current menu UI live
            ui->MenuListWidget->takeItem(index);
            // go back if no items left
            if (ui->MenuListWidget->count() == 0) {
                back_button();
            }
        }
        catch (QString error) {
            print(error);
        }
        return;
    }

    // If we are in the reset screen, clear all sessions
    if (current_menu->get_name() == "RESET") {
        print("Resetting device");
        reset_device();
        return;
    }

    // If we are in View, show the summary for the session of the currently selected ID
    if (current_menu->get_name() == "VIEW") {
        int session_id = string_to_id(current_menu->get_item(index)->get_name());
        if (session_id == -1) {
            print("Error: invalid index");
        }
        Session* currSesh = db.get_session_by_id(session_id);

        if (currSesh == nullptr) {
            print("Could not get session with id: " + QString::number(session_id));
            return;
        }
        update_summary(currSesh->low_percentage, currSesh->med_percentage, currSesh->high_percentage, currSesh->avg_coherence,
                       currSesh->achievement_score, currSesh->total_time);
        create_summary_graph(currSesh->time_data, currSesh->bpm_data);
        show_summary();

        current_menu = current_menu->get_item(index);
        change_menu("VIEW SESSION", {});
        return;
    }

    // If no sub-menu, do nothing
    if (current_menu->get_item(index)->get_items().size() == 0) {
        return;
    }

    // If sub-menu exists, navigate to selected sub-menu
    if (current_menu->get_item(index)->get_items().size() > 0) {
        current_menu = current_menu->get_item(index);
        change_menu(current_menu->get_name(), current_menu->get_items());
    }
}

void MainWindow::back_button() {

    if (summary_visible) {
        // Exit summary and go to view menu, if we are viewing a summary from history view
        if (current_menu->get_name() == "VIEW SESSION") {
            hide_summary();
            current_menu = session_view;
            change_menu(current_menu->get_name(), current_menu->get_items());
            show_menu();
        }
        else {
            // Exit summary and go to main menu, this is for summary view after new session
            hide_summary();
            current_menu = current_menu->get_parent();
            change_menu(current_menu->get_name(), current_menu->get_items());
            show_menu();
        }
        return;
    }
    
    // if we are not in main, go back up one level
    if (current_menu->get_parent() != nullptr) {
        if (current_menu->get_name() == "NEW SESSION") {
            if(!stop_session()) { // if we don't save the session, go straight to main menu
                current_menu = current_menu->get_parent();
                change_menu(current_menu->get_name(), current_menu->get_items());
                show_menu();
            }
        }
        else {
            current_menu = current_menu->get_parent();
            change_menu(current_menu->get_name(), current_menu->get_items());
            show_menu();
        }
    }
}

void MainWindow::menu_button() {
    // Exit summary
    if (summary_visible) {
        hide_summary();
        current_menu = main_menu;
        change_menu(main_menu->get_name(), main_menu->get_items());
    }

    if (current_menu->get_name() == "NEW SESSION") {
        if (!stop_session()) { // if we don't save the session, go straight to main menu
            current_menu = main_menu;
            change_menu(main_menu->get_name(), main_menu->get_items());
            show_menu();
        }
    } else {
        current_menu = main_menu;
        change_menu(main_menu->get_name(), main_menu->get_items());
        show_menu();
    }
}

// disables/enables all buttons and blocks/unblocks screen depending on power state
void MainWindow::power_button() {
    // if the battery level is 0, it does not turn on
    if (battery_level == 0) {
        if(power_on) {
            print("Out of battery, shutting off!");
            change_power();
        }
        else {
            print("Out of battery, please charge device!");
        }
        return;
    }

    change_power();
}

void MainWindow::charge_button() {
    update_battery_level(100);
}

void MainWindow::set_power_state(bool on) {
    if (power_on == on) return;
    power_button();
}

void MainWindow::change_power() {
    power_on = !power_on;
    ui->up_button->setEnabled(power_on);
    ui->down_button->setEnabled(power_on);
    ui->left_button->setEnabled(power_on);
    ui->right_button->setEnabled(power_on);
    ui->ok_button->setEnabled(power_on);
    ui->menu_button->setEnabled(power_on);
    ui->back_button->setEnabled(power_on);
    ui->screen_blocker->setStyleSheet(power_on ? "" : "background-color: black;");
    update_coherence();
    menu_button();
}

void MainWindow::update_battery_level(int value) {
    battery_level = value;
    ui->battery_display->setValue(battery_level);
    ui->batterylevel->setValue(battery_level);

    // sets battery display color according to value
    if (battery_level >= 40) {
        ui->battery_display->setStyleSheet("QProgressBar { selection-background-color: green; background-color: black; color: white; }");
    }
    else if (battery_level >= 15) {
        ui->battery_display->setStyleSheet("QProgressBar { selection-background-color: orange;  background-color: black; color: white; }");
    }
    else {
        ui->battery_display->setStyleSheet("QProgressBar { selection-background-color: red; background-color: black; color: white; }");
    }
    // Ensures that device cannot be on with 0% battery
    if (battery_level <= 0) {
        set_power_state(false);
    }
}

void MainWindow::update_timer() {
    // Coherence
    update_coherence();

    coherence_timer++;

    // Every 5 seconds, get the total sum of coherence scores sampled every 5 seconds
    // and update the coherence, and also decrement the battery level
    if ((coherence_timer % (framerate * 5)) == 0) {
        update_coherence_level();
        achievement_score += coherence;
        update_battery_level(battery_level - 1);
    }

    // Breath Pacer
    int current_value = ui->breath_pacer->value();

    if (slider_moving_right) {
        current_value++;
        if (current_value == ui->breath_pacer->maximum()) {
            slider_moving_right = false;
        }
    } else {
        current_value--;
        if (current_value == ui->breath_pacer->minimum()) {
            slider_moving_right = true;
        }
    }
    ui->breath_pacer->setValue(current_value);

    if (current_value % framerate == 0) {
        // HRV Plot
        update_hrv_graph();
        // Update QLabels
        update_labels();
    }
}

void MainWindow::update_coherence() {
    // does not display coherence level when device is off
    if (!power_on) {
        ui->coherence_range_color->setStyleSheet("background-color: black; color: white;");
        ui->coherence_range_color->setText("Off");
        return;
    }

    // sets corresponding coherence color depending on current challenge level thresholds
    if (coherence >= high_threshold) {
        ui->coherence_range_color->setStyleSheet("background-color: green; color: white;");
        ui->coherence_range_color->setText("High");
    } else if (coherence >= medium_threshold) {
        ui->coherence_range_color->setStyleSheet("background-color: blue; color: white;");
        ui->coherence_range_color->setText("Med");
    } else {
        ui->coherence_range_color->setStyleSheet("background-color: red; color: white;");
        ui->coherence_range_color->setText("Low");
    }
}

void MainWindow::set_coherence(double ch) {
    print("Coherence changed: beep!");
    coherence = ch;
}

void MainWindow::set_challenge_level(int level) {
    switch(level) {
        case 1:
            medium_threshold = 0.5;
            high_threshold = 0.9;
            break;
        case 2:
            medium_threshold = 0.6;
            high_threshold = 2.1;
            break;
        case 3:
            medium_threshold = 1.8;
            high_threshold = 4.0;
            break;
        case 4:
            medium_threshold = 4.0;
            high_threshold = 6.0;
            break;
        default:
            print("Invalid challenge level!");
            return;
    }
    challenge_level = level;
    update_coherence();
}

void MainWindow::set_breath_pacer_interval(int interval) {
    if (interval >= 1 and interval <= 30) {
        breath_pace = interval;
    }
    else {
        print("invalid interval!");
    }
}

void MainWindow::update_breath_slider() {
    ui->breath_pacer->setMaximum(breath_pace);
    reset_breath_slider();
}

// Reset the breath slider to min position
void MainWindow::reset_breath_slider() {
    slider_moving_right = true;
    ui->breath_pacer->setValue(ui->breath_pacer->minimum());
}

void MainWindow::print(const QString& line) {
    ui->console_output->append(line); // prints line to console window
    qInfo("%s", line.toStdString().c_str()); // prints line to terminal
}

void MainWindow::initialize_menus(Menu* main) {

    // Menus in main menu
    Menu* start = new Menu("NEW SESSION", main);
    Menu* history = new Menu("HISTORY", main);
    Menu* settings = new Menu("SETTINGS", main);
    Menu* reset = new Menu("RESET", main);
    main->add_sub_menu(start, start->get_name());
    main->add_sub_menu(history, history->get_name());
    main->add_sub_menu(settings, settings->get_name());
    main->add_sub_menu(reset, reset->get_name());

    // History sub menus
    Menu* viewHistory = new Menu("VIEW", history);
    session_view = viewHistory;
    Menu* deleteHistory = new Menu("DELETE", history);
    delete_session_view = deleteHistory;
    history->add_sub_menu(viewHistory, viewHistory->get_name());
    history->add_sub_menu(deleteHistory, deleteHistory->get_name());

    // append sessions to viewHistory and deleteHistory in chronological order
    for (int i = sessions.size() - 1; i >= 0; i--) {
        QString name = QString::number(sessions.at(i)->session_id) + ": Session on " + QString(sessions.at(i)->date_time.toString());
        Menu* newSesh = new Menu(name, viewHistory);
        viewHistory->add_sub_menu(newSesh, newSesh->get_name());

        Menu* newSeshDel = new Menu(name, deleteHistory);
        deleteHistory->add_sub_menu(newSeshDel, newSeshDel->get_name());

    }

    // Settings sub menus
    Menu* breathPacer = new Menu("BREATH PACER", settings);
    Menu* level = new Menu("LEVEL", settings);
    settings->add_sub_menu(breathPacer, breathPacer->get_name());
    settings->add_sub_menu(level, level->get_name());

    // Level sub menus
    Menu* beginner = new Menu("BEGINNER", level);
    Menu* intermediate = new Menu("INTERMEDIATE", level);
    Menu* advanced = new Menu("ADVANCED", level);
    Menu* expert = new Menu("EXPERT", level);
    level->add_sub_menu(beginner, beginner->get_name());
    level->add_sub_menu(intermediate, intermediate->get_name());
    level->add_sub_menu(advanced, advanced->get_name());
    level->add_sub_menu(expert, expert->get_name());

    // Breath pacer sub menus
    for (int i = 1; i <= 30; i++) {
        Menu* newNum = new Menu(QString::number(i), breathPacer);
        breathPacer->add_sub_menu(newNum, newNum->get_name());
    }

    // Reset sub menu
    Menu* confirm = new Menu("CONFIRM RESET", reset);
    reset->add_sub_menu(confirm, confirm->get_name());
}

void MainWindow::change_menu(const QString menuName, const QStringList items) {
    active_qlist->clear();
    if (!items.empty()) {
        active_qlist->addItems(items);
        active_qlist->setCurrentRow(0);
    }

    ui->MenuLabel->setText(menuName);
}

void MainWindow::initialize_timer() {
    slider_moving_right = true;
    ui->breath_pacer->setMaximum(breath_pace*framerate);
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update_timer()));
}

void MainWindow::create_hrv_graph() {
    bpm_file_pos = 0;
    initial_range = 10.0;
    plot->addGraph();
    plot->xAxis->setLabel("Time (seconds)");
    plot->yAxis->setLabel("Heart Rate");

    plot->xAxis->setRange(0, initial_range);
    plot->yAxis->setRange(50, 100);

    // Style properties (solid line, blue, circle filled with pen colour)
    plot->graph(0)->setLineStyle(QCPGraph::lsLine);
    plot->graph(0)->setPen(QPen(Qt::blue));
    plot->graph(0)->setScatterStyle(QCPScatterStyle::ssDisc);

    plot->replot();
    update_hrv_graph();
}

void MainWindow::update_hrv_graph() {
    QFile file(bpm_file);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        print("failed to read file");
        return;
    }

    QTextStream in(&file);
    // Set stream position to bpm_file_pos (line last left off)
    in.seek(bpm_file_pos);
    if (!in.atEnd()) {
        double hr = in.readLine().toDouble();
        if (hr < 0) {
            print("Lost HR Contact");
            // Back button calls the stop session
            change_hr_contact();
            back_button();
            return;
        }
        heart_rate.append(hr);
        time.append(time.size());
    } else {
        print("Reached end of BPM file");
        stop_session(); // if done reading file, we stop the session
        return;
    }
    plot->graph(0)->setData(time, heart_rate);

    // Adjusting x range to show only last 10 seconds of data
    double x_max = time.last();
    double x_min = qMax(0.0, x_max - 10.0);
    plot->xAxis->setRange(x_min, x_max);

    plot->replot();
    bpm_file_pos = in.pos();
}

void MainWindow::end_hrv_graph() {
    time.clear();
    heart_rate.clear();
    bpm_file_pos = 0;
}

void MainWindow::reset_coherence() {
    coherence_file_pos = 0;
    coherence_timer = 0;
}

void MainWindow::stop_coherence() {
    reset_coherence();
    coherences.clear();
}

void MainWindow::update_coherence_level() {
    QFile file(coherence_file);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        print("failed to read file");
        qDebug() << file.errorString();
        return;
    }


    QTextStream in(&file);
    // Set stream position to coherence_file_pos (line last left off)
    in.seek(coherence_file_pos);
    if (!in.atEnd()) {
        double coherence = in.readLine().toDouble();
        coherences.append(coherence);
        set_coherence(coherence);
    }
    else {
        print("Reached end of coherence file");
    }

    coherence_file_pos = in.pos();
}

void MainWindow::update_labels() {
    ui->coherence_label->setText("Coherence\n" + QString::number(coherence));
    if (!time.isEmpty()) {
        int seconds = time.back();
        int minutes = seconds / 60;
        seconds %= 60;
        // field width 2, base 10 (if minutes or seconds < 10, it will have a leading '0' character)
        QString formatted_time = QString("%1:%2").arg(minutes, 2, 10, QLatin1Char('0')).arg(seconds, 2, 10, QLatin1Char('0'));
        ui->length_label->setText("Length\n" + formatted_time);
    }

    ui->achievement_label->setText("Achievement\n" + QString::number(achievement_score));
}

void MainWindow::start_session() {
    reset_coherence();
    initialize_timer();
    timer->start(1000/framerate);
    create_hrv_graph();
    update_labels();
    change_hr_contact();
}

void MainWindow::reset_device() {
    db.delete_all_sessions();
    session_view->delete_all_sub_menus();
    delete_session_view->delete_all_sub_menus();
    current_menu = current_menu->get_parent();
    change_menu(current_menu->get_name(), current_menu->get_items());
    set_challenge_level(1);
    set_breath_pacer_interval(10);
}

void MainWindow::change_hr_contact() {
    if (hr_contact) {
        hr_contact = false;
        ui->hr_contact->setPixmap(QPixmap(":/img/sensors/sensor_off.png"));
    } else {
        hr_contact = true;
        ui->hr_contact->setPixmap(QPixmap(":/img/sensors/sensor_on.png"));
    }
}

QVector<double> MainWindow::calculate_coherence_percentages() {
    QVector<double> percentages;

    // keep track of how many seconds are in each coherence level
    double lowSeconds = 0;
    double medSeconds = 0;
    double highSeconds = 0;

    for (int i = 0; i < coherences.size(); i++) {
        if (coherences.at(i) < medium_threshold) {
            lowSeconds++;
        }
        else if (coherences.at(i) >= medium_threshold && coherences.at(i) < high_threshold) {
            medSeconds++;
        }
        else { // coherences.at(i) >= high_threshold
            highSeconds++;
        }
    }

    // calculate and append percentages to return array
    percentages.append(lowSeconds/coherences.size()*100);
    percentages.append(medSeconds/coherences.size()*100);
    percentages.append(highSeconds/coherences.size()*100);

    return percentages;
}

double MainWindow::calculate_mean_coherence() {
    double mean = 0;

    for (int i = 0; i < coherences.size(); i++) {
        mean += coherences.at(i);
    }

    return mean/coherences.size();
}

bool MainWindow::stop_session() {
    double low_percentage = 0;
    double med_percentage = 0;
    double high_percentage = 0;
    double avg_coherence = 0;
    QVector<double> percentages;

    // if the session ended due to power off, do not save
    if (!power_on) {
        print("Unexpected power off, failed to save session");
        change_hr_contact();
        reset_values_after_session();
        return false;
    }

    // if the session ended due to HR contact lost, do not save
    if (!hr_contact) {
        print("HR Contact lost, failed to save session");
        reset_values_after_session();
        return false;
    }

    // if not enough data, do not save
    if (coherences.size() < 1) {
        print("Not enough data, failed to save session");
        change_hr_contact();
        reset_values_after_session();
        return false;
    }

    percentages = calculate_coherence_percentages();
    avg_coherence = calculate_mean_coherence();

    // if there are not exactly 3 percentages, corresponding to low,med and high, do not save
    if (percentages.size() != 3) {
        print("Error calculating percentages, failed to save session");
        change_hr_contact();
        reset_values_after_session();
        return false;
    }

    low_percentage = percentages.at(0);
    med_percentage = percentages.at(1);
    high_percentage = percentages.at(2);

    if (!db.add_session(challenge_level, low_percentage, med_percentage, high_percentage, avg_coherence, time.back(), achievement_score, time, heart_rate)) {
        print("Could not add to DB, failed to save session");
        change_hr_contact();
        reset_values_after_session();
        return false;
    }

    sessions = db.get_all_sessions();

    // update the view menu
    QString name = QString::number(sessions.back()->session_id) + ": Session on " + QString(sessions.back()->date_time.toString());
    Menu* newSesh = new Menu(name, session_view);
    session_view->add_sub_menu_front(newSesh, newSesh->get_name());
    Menu* newSeshDel = new Menu(name, delete_session_view);
    delete_session_view->add_sub_menu_front(newSeshDel, newSeshDel->get_name());

    update_summary(low_percentage, med_percentage, high_percentage, avg_coherence, achievement_score, time.back());
    create_summary_graph(time, heart_rate);
    show_summary();

    change_hr_contact();
    reset_values_after_session();
    return true;
}

void MainWindow::reset_values_after_session() {
    achievement_score = 0;
    reset_breath_slider();
    timer->stop();
    set_coherence(0.00);
    stop_coherence();
    end_hrv_graph();
}

void MainWindow::show_menu() {
    ui->achievement_label->setVisible(false);
    ui->length_label->setVisible(false);
    ui->coherence_label->setVisible(false);
    ui->hrv_graph->setVisible(false);
    ui->coherence_range_color->setVisible(false);
    ui->breath_pacer->setVisible(false);
    ui->MenuListWidget->setVisible(true);
}

void MainWindow::show_summary() {
    summary_visible = true;

    ui->achievement_label->setVisible(false);
    ui->length_label->setVisible(false);
    ui->coherence_label->setVisible(false);
    ui->hrv_graph->setVisible(false);
    ui->coherence_range_color->setVisible(false);
    ui->breath_pacer->setVisible(false);
    ui->MenuListWidget->setVisible(false);

    ui->summary_label->setVisible(true);
    ui->summary_arrow_indicator->setVisible(true);

    // Summary Values
    if (summary_page == 0) {

        ui->summary_hrv_graph->setVisible(false);

        ui->summary_level_box->setVisible(true);
        ui->summary_low_box->setVisible(true);
        ui->summary_med_box->setVisible(true);
        ui->summary_high_box->setVisible(true);
        ui->summary_avg_coh_box->setVisible(true);
        ui->summary_len_box->setVisible(true);
        ui->summary_achievement_box->setVisible(true);
        ui->summary_level_label->setVisible(true);
        ui->summary_low_label->setVisible(true);
        ui->summary_med_label->setVisible(true);
        ui->summary_high_label->setVisible(true);
        ui->summary_avg_coh_label->setVisible(true);
        ui->summary_len_label->setVisible(true);
        ui->summary_achievement_label->setVisible(true);
        ui->summary_arrow_indicator->setStyleSheet("border-image: url(:/img/buttons/rightbutton.png);");

        ui->summary_level_value->setVisible(true);
        ui->summary_low_value->setVisible(true);
        ui->summary_med_value->setVisible(true);
        ui->summary_high_value->setVisible(true);
        ui->summary_avg_coh_value->setVisible(true);
        ui->summary_len_value->setVisible(true);
        ui->summary_achievement_value->setVisible(true);
    } else { // Summary Graph
        ui->summary_level_box->setVisible(false);
        ui->summary_low_box->setVisible(false);
        ui->summary_med_box->setVisible(false);
        ui->summary_high_box->setVisible(false);
        ui->summary_avg_coh_box->setVisible(false);
        ui->summary_len_box->setVisible(false);
        ui->summary_achievement_box->setVisible(false);
        ui->summary_level_label->setVisible(false);
        ui->summary_low_label->setVisible(false);
        ui->summary_med_label->setVisible(false);
        ui->summary_high_label->setVisible(false);
        ui->summary_avg_coh_label->setVisible(false);
        ui->summary_len_label->setVisible(false);
        ui->summary_achievement_label->setVisible(false);
        ui->summary_arrow_indicator->setStyleSheet("border-image: url(:/img/buttons/leftbutton.png);");

        ui->summary_level_value->setVisible(false);
        ui->summary_low_value->setVisible(false);
        ui->summary_med_value->setVisible(false);
        ui->summary_high_value->setVisible(false);
        ui->summary_avg_coh_value->setVisible(false);
        ui->summary_len_value->setVisible(false);
        ui->summary_achievement_value->setVisible(false);

        ui->summary_hrv_graph->setVisible(true);

    }
}

void MainWindow::hide_summary() {
    summary_visible = false;
    summary_page = 0;

    ui->summary_label->setVisible(false);
    ui->summary_level_box->setVisible(false);
    ui->summary_low_box->setVisible(false);
    ui->summary_med_box->setVisible(false);
    ui->summary_high_box->setVisible(false);
    ui->summary_avg_coh_box->setVisible(false);
    ui->summary_len_box->setVisible(false);
    ui->summary_achievement_box->setVisible(false);
    ui->summary_level_label->setVisible(false);
    ui->summary_low_label->setVisible(false);
    ui->summary_med_label->setVisible(false);
    ui->summary_high_label->setVisible(false);
    ui->summary_avg_coh_label->setVisible(false);
    ui->summary_len_label->setVisible(false);
    ui->summary_achievement_label->setVisible(false);
    ui->summary_arrow_indicator->setVisible(false);

    ui->summary_level_value->setVisible(false);
    ui->summary_low_value->setVisible(false);
    ui->summary_med_value->setVisible(false);
    ui->summary_high_value->setVisible(false);
    ui->summary_avg_coh_value->setVisible(false);
    ui->summary_len_value->setVisible(false);
    ui->summary_achievement_value->setVisible(false);

    ui->summary_hrv_graph->setVisible(false);
}

void MainWindow::update_summary(double low_percentage, double med_percentage, double high_percentage, double avg_coherence, float achievement_score, int seconds) {
    ui->summary_level_value->setText(QString::number(challenge_level));
    ui->summary_low_value->setText(QString::number(static_cast<int>(low_percentage)) + "%");
    ui->summary_med_value->setText(QString::number(static_cast<int>(med_percentage)) + "%");
    ui->summary_high_value->setText(QString::number(static_cast<int>(high_percentage)) + "%");
    ui->summary_avg_coh_value->setText(QString::number(avg_coherence));

    int minutes = seconds / 60;
    seconds %= 60;
    QString formatted_time = QString("%1:%2").arg(minutes, 2, 10, QLatin1Char('0')).arg(seconds, 2, 10, QLatin1Char('0'));

    ui->summary_len_value->setText(formatted_time);
    ui->summary_achievement_value->setText(QString::number(achievement_score));

}

void MainWindow::create_summary_graph(QVector<double> time, QVector<double> heart_rate) {
    QCustomPlot* summary_plot;
    summary_plot = ui->summary_hrv_graph;

    summary_plot->addGraph();
    summary_plot->graph(0)->setData(time, heart_rate);
    summary_plot->xAxis->setLabel("Time (seconds)");
    summary_plot->yAxis->setLabel("Heart Rate");
    summary_plot->xAxis->setRange(0, time.last());
    summary_plot->yAxis->setRange(50, 100);
    summary_plot->graph(0)->setLineStyle(QCPGraph::lsLine);
    summary_plot->graph(0)->setPen(QPen(Qt::blue));
    summary_plot->graph(0)->setScatterStyle(QCPScatterStyle::ssDisc);
    summary_plot->replot();
    summary_plot->show();
}

void MainWindow::set_input(file_names index) {
    if (index < low || index >= num) {
        index = short_test;
        print("ERROR: Invalid index, defaulting to short_test");
    }
    QString files[num] = {"low", "med", "high", "mixed", "short_test", "hr_contact_lost"};
    bpm_file = ":/data/" + files[index] + ".txt";
    coherence_file = ":data/" + files[index] + "_coherence.txt";
    print("BPM source set to " + bpm_file);
    print("Coherence source set to " + coherence_file);
}

int MainWindow::string_to_id(QString item_name) {
    int colonPos = item_name.indexOf(':');
    if (colonPos != -1) {
        QStringRef numString = item_name.leftRef(colonPos);
        bool ok;
        int numValue = numString.toInt(&ok);
        if (ok) {
            return numValue;
        }
    }
    return -1; // return -1 if a colon is not found or the value before the colon is not a valid number
}
