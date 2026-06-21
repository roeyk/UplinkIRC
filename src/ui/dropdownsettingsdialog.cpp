#include "dropdownsettingsdialog.h"
#include "ui/solidcombobox.h"

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

namespace {

QSlider *makePercentSlider(int value, int minValue = 20, int maxValue = 100)
{
    auto *slider = new QSlider(Qt::Horizontal);
    slider->setRange(minValue, maxValue);
    slider->setSingleStep(5);
    slider->setPageStep(10);
    slider->setValue(qBound(minValue, value, maxValue));
    return slider;
}

QLabel *valueLabel(QSlider *slider, const QString &suffix)
{
    auto *label = new QLabel(QString::number(slider->value()) + suffix);
    QObject::connect(slider, &QSlider::valueChanged, label, [label, suffix](int value) {
        label->setText(QString::number(value) + suffix);
    });
    return label;
}

QWidget *sliderRow(QSlider *slider, const QString &suffix)
{
    auto *row = new QWidget;
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(slider, 1);
    layout->addWidget(valueLabel(slider, suffix));
    return row;
}

} // namespace

DropdownSettingsDialog::DropdownSettingsDialog(const UiConfig &ui, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Dropdown Settings"));
    setMinimumWidth(460);

    auto *root = new QVBoxLayout(this);

    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_shortcutEdit = new QKeySequenceEdit(QKeySequence(ui.dropdownShortcut));
    form->addRow(tr("WM shortcut:"), m_shortcutEdit);

    m_edgeCombo = new SolidComboBox;
    m_edgeCombo->addItem(tr("Top"), QStringLiteral("top"));
    m_edgeCombo->addItem(tr("Bottom"), QStringLiteral("bottom"));
    m_edgeCombo->addItem(tr("Left"), QStringLiteral("left"));
    m_edgeCombo->addItem(tr("Right"), QStringLiteral("right"));
    const int edgeIndex = m_edgeCombo->findData(ui.dropdownEdge.toLower());
    m_edgeCombo->setCurrentIndex(edgeIndex >= 0 ? edgeIndex : 0);
    form->addRow(tr("Slide from:"), m_edgeCombo);

    m_widthSlider = makePercentSlider(ui.dropdownWidthPercent, 10, 100);
    form->addRow(tr("Width:"), sliderRow(m_widthSlider, QStringLiteral("%")));

    m_heightSlider = makePercentSlider(ui.dropdownHeightPercent, 10, 100);
    form->addRow(tr("Height:"), sliderRow(m_heightSlider, QStringLiteral("%")));

    m_animationSlider = makePercentSlider(ui.dropdownAnimationMs, 0, 1000);
    m_animationSlider->setSingleStep(25);
    m_animationSlider->setPageStep(100);
    form->addRow(tr("Slide time:"), sliderRow(m_animationSlider, QStringLiteral(" ms")));

    m_opacitySlider = makePercentSlider(ui.dropdownOpacityPercent);
    form->addRow(tr("Focused opacity:"), sliderRow(m_opacitySlider, QStringLiteral("%")));

    m_inactiveOpacitySlider = makePercentSlider(ui.dropdownInactiveOpacityPercent);
    form->addRow(tr("Defocused opacity:"), sliderRow(m_inactiveOpacitySlider, QStringLiteral("%")));

    root->addLayout(form);

    m_commandLabel = new QLabel;
    m_commandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_commandLabel->setWordWrap(true);
    root->addWidget(m_commandLabel);

    auto *copyButton = new QPushButton(tr("Copy WM Command"));
    copyButton->setAutoDefault(false);
    root->addWidget(copyButton);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    root->addWidget(buttons);

    connect(m_edgeCombo, &QComboBox::currentIndexChanged, this, [this] { refreshCommand(); });
    connect(m_shortcutEdit, &QKeySequenceEdit::keySequenceChanged, this, [this] { refreshCommand(); });
    connect(copyButton, &QPushButton::clicked, this, [this] {
        qApp->clipboard()->setText(QStringLiteral("Uplink --toggle-dropdown --dropdown-edge %1").arg(edge()));
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    refreshCommand();
}

QString DropdownSettingsDialog::shortcut() const
{
    return m_shortcutEdit->keySequence().toString(QKeySequence::PortableText);
}

QString DropdownSettingsDialog::edge() const
{
    return m_edgeCombo->currentData().toString();
}

int DropdownSettingsDialog::widthPercent() const
{
    return m_widthSlider->value();
}

int DropdownSettingsDialog::heightPercent() const
{
    return m_heightSlider->value();
}

int DropdownSettingsDialog::animationMs() const
{
    return m_animationSlider->value();
}

int DropdownSettingsDialog::opacityPercent() const
{
    return m_opacitySlider->value();
}

int DropdownSettingsDialog::inactiveOpacityPercent() const
{
    return m_inactiveOpacitySlider->value();
}

void DropdownSettingsDialog::refreshCommand()
{
    m_commandLabel->setText(tr("Bind %1 in your window manager to: %2")
        .arg(shortcut().isEmpty() ? tr("your chosen shortcut") : shortcut(),
             QStringLiteral("Uplink --toggle-dropdown --dropdown-edge %1").arg(edge())));
}
