# include "menu.h"

Menu::Menu(QString name, Menu *parent): name(name), parent(parent) {}

Menu::~Menu() {
    for (int i = 0; i < sub_menus.length(); i++) {
        delete sub_menus.at(i);
    }
}

void Menu::add_sub_menu(Menu *m, QString name) {
    sub_menus.push_back(m);
    sub_menu_names.push_back(name);
}

void Menu::add_sub_menu_front(Menu *m, QString name) {
    sub_menus.push_front(m);
    sub_menu_names.push_front(name);
}

void Menu::remove_sub_menu(QString name) {

    int idx = sub_menu_names.indexOf(name);
    sub_menu_names.removeAll(name);
    Menu* temp = sub_menus.at(idx);
    sub_menus.remove(idx);
    delete temp;
}

void Menu::delete_all_sub_menus() {
    sub_menus.clear();
    sub_menu_names.clear();
}

void Menu::print(const QString& line) {
    qInfo("%s", line.toStdString().c_str()); // prints line to terminal
}

// Getters and Setters
QString Menu::get_name() { return name; }

QStringList Menu::get_items() { return sub_menu_names; }

Menu* Menu::get_parent() { return parent; }

Menu* Menu::get_item(int idx) { return sub_menus.at(idx); }
