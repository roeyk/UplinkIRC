#pragma once
#include <QAbstractItemView>
#include <QComboBox>
#include <QFontComboBox>

// QComboBox subclass that forces the popup container to have a solid background.
// The popup container (QComboBoxPrivateContainer) is a separate top-level window
// that is only created inside showPopup(). Stylesheets and autoFillBackground set
// before that point don't reach it. Overriding showPopup() and setting the
// container's background after Qt creates it is the only reliable fix.

class SolidComboBox : public QComboBox {
public:
    using QComboBox::QComboBox;
protected:
    void showPopup() override {
        QComboBox::showPopup();
        if (QWidget *container = view()->parentWidget()) {
            container->setAutoFillBackground(true);
            container->update();
        }
    }
};

class SolidFontComboBox : public QFontComboBox {
public:
    using QFontComboBox::QFontComboBox;
protected:
    void showPopup() override {
        QFontComboBox::showPopup();
        if (QWidget *container = view()->parentWidget()) {
            container->setAutoFillBackground(true);
            container->update();
        }
    }
};
