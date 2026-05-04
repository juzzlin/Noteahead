#ifndef SAMPLER_PAD_MODEL_HPP
#define SAMPLER_PAD_MODEL_HPP

#include <QAbstractListModel>
#include <memory>
#include <vector>

#include "../../../domain/devices/sampler_device.hpp"

namespace noteahead {

class SamplerPadModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        Note = Qt::UserRole + 1,
        NoteName,
        FilePath,
        IsLoaded
    };
    Q_ENUM(Roles)

    explicit SamplerPadModel(SamplerDevice::SamplerDeviceS sampler, QObject * parent = nullptr);

    void setSampler(SamplerDevice::SamplerDeviceS sampler);

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void updatePad(int padIndex);

private:
    SamplerDevice::SamplerDeviceS m_sampler;
    static constexpr int PadCount = 16;
    static constexpr int StartNote = 36;
};

} // namespace noteahead

#endif // SAMPLER_PAD_MODEL_HPP
