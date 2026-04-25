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

public:
    explicit SamplerController(std::shared_ptr<SamplerDevice> sampler, QObject * parent = nullptr);
    ~SamplerController() override;

    SamplerPadModel * padModel() const;
    std::shared_ptr<SamplerDevice> sampler() const;

    Q_INVOKABLE void loadSample(int padIndex, const QString & filePath);
    Q_INVOKABLE void clearSample(int padIndex);
    Q_INVOKABLE void playSample(int padIndex, double velocity = 1.0);

private:
    std::shared_ptr<SamplerDevice> m_sampler;
    std::unique_ptr<SamplerPadModel> m_padModel;
};

} // namespace noteahead

#endif // SAMPLER_CONTROLLER_HPP
