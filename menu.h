#ifndef MENU_H
#define MENU_H

#include <QString>
#include <QStringList>
#include <QVector>

class Menu {

public:
    Menu(QString name, Menu* parent);
    ~Menu();

    // Getters
    QString get_name();
    QStringList get_items();
    Menu* get_parent();
    Menu* get_item(int idx);

    // Add a sub menu
    void add_sub_menu(Menu* m, QString name);

    // Add a sub menu but to the front of the QVector
    void add_sub_menu_front(Menu* m, QString name);

    //Remove the sub menu with the given name
    void remove_sub_menu(QString name);

    // Clear all sub menus
    void delete_all_sub_menus();

private:
    QString name;
    QStringList sub_menu_names;
    QVector<Menu*> sub_menus;
    Menu* parent;
    void print(const QString& line);

};


#endif // MENU_H
