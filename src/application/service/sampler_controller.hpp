#ifndef SAMPLER_CONTROLLER_HPP
#define SAMPLER_CONTROLLER_HPP

#include <QObject>
#include <memory>

namespace noteahead {

class SamplerDevice;
class SamplerPadModel;

class SamplerController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(noteahead::SamplerPadModel * padModel READ padModel CONSTANT)
    Q_PROPERTY(int selectedPad READ selectedPad WRITE setSelectedPad NOTIFY selectedPadChanged)

public:
    explicit SamplerController(std::shared_ptr<SamplerDevice> sampler, QObject * parent = nullptr);
    ~SamplerController() override;

    SamplerPadModel * padModel() const;
    std::shared_ptr<SamplerDevice> sampler() const;

    int selectedPad() const;
    void setSelectedPad(int selectedPad);

    Q_INVOKABLE QVariantList getWaveformData(int numPoints);

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void accept();
    Q_INVOKABLE void reject();

    Q_INVOKABLE void loadSample(int padIndex, const QString & filePath);
    Q_INVOKABLE void clearSample(int padIndex);
    Q_INVOKABLE void playSample(int padIndex, double velocity = 1.0);

signals:
    void selectedPadChanged();

private:
    std::shared_ptr<SamplerDevice> m_sampler;
    std::unique_ptr<SamplerPadModel> m_padModel;
    int m_selectedPad = 0;
};

} // namespace noteahead

#endif // SAMPLER_CONTROLLER_HPP
