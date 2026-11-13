// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

#ifndef ROOM_ACOUSTICS_HPP
#define ROOM_ACOUSTICS_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace noteahead::RoomAcoustics {

//! The octave bands everything frequency-dependent is worked out in, 125 Hz to 8 kHz.
constexpr size_t BandCount { 7 };
using Bands = std::array<double, BandCount>;
constexpr Bands BandHz { 125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0 };

constexpr double SpeedOfSound { 343.0 };

//! What a surface is made of. The ordinal is what a project stores: append only.
enum class Material
{
    Stone,
    Concrete,
    Brick,
    Plaster,
    Wood,
    Glass,
    Carpet,
    Curtain,
    AcousticPanel,
    Audience
};
constexpr size_t MaterialCount { 10 };

//! The share of the energy hitting the surface that it absorbs, per band: the textbook
//! coefficients, the 4 kHz one carried on to 8 kHz.
const Bands & absorption(Material material);

//! What air takes off over a metre, in decibels, per band: ISO 9613-1 at 20 degrees and 70 %.
const Bands & airAbsorptionDbPerMetre();

//! A microphone's polar pattern. The ordinal is what a project stores: append only.
enum class Pattern
{
    Omni,
    Cardioid,
    FigureEight
};
constexpr size_t PatternCount { 3 };

//! The pattern's gain for sound arriving at an angle whose cosine to the mic's axis is @p cosine.
//! Signed: a figure-eight hears its back lobe inverted.
double patternGain(Pattern pattern, double cosine);

//! How much of a diffuse field the pattern picks up against an omni: 1, 1/3 and 1/3. What the
//! late tail is scaled by, the tail arriving from every direction at once.
double randomEnergyEfficiency(Pattern pattern);

struct Vec3
{
    double x { 0.0 };
    double y { 0.0 };
    double z { 0.0 };
};

//! The mic pairs. Close follows the source; Room and Far stand where they are put.
enum class MicGroup
{
    Close,
    Room,
    Far
};
constexpr size_t MicGroupCount { 3 };

//! Everything the room's impulse responses depend on, in metres, degrees and linear gain.
//!
//! The source and the Room and Far pairs are placed as fractions of the room's width and depth, so
//! that resizing the room keeps the layout. The Close pair is placed by its offset from the source
//! in metres, so that moving the source takes its close mics with it. The room is seen from above,
//! with y running from the front wall, behind the source, to the back wall.
struct RoomSettings
{
    double widthM { 9.0 };
    double depthM { 7.0 };
    double heightM { 3.5 };
    Material walls { Material::Wood };
    Material floor { Material::Wood };
    Material ceiling { Material::AcousticPanel };

    double sourceX { 0.5 };
    double sourceY { 0.3 };
    //! The distance between the left and the right source, the two channels of the input.
    double sourceSpreadM { 1.5 };

    struct Group
    {
        //! A fraction of the room, or for Close the offset from the source in metres.
        double x { 0.0 };
        double y { 0.0 };
        //! The distance between the pair. Close has none of its own: its mics are on the sources.
        double spacingM { 0.0 };
        //! How far each mic of the pair turns away from the source, outwards.
        double angleDeg { 0.0 };
        Pattern pattern { Pattern::Omni };
        double level { 1.0 };
    };

    std::array<Group, MicGroupCount> groups {};

    //! Whether two settings give the same impulse responses. The levels only scale them.
    bool sameGeometry(const RoomSettings & other) const;
    bool sameLevels(const RoomSettings & other) const;
};

struct Mic
{
    Vec3 position;
    //! A unit vector along the mic's axis, level with the floor.
    Vec3 aim;
    Pattern pattern { Pattern::Omni };
};

//! Where everything is, worked out from the settings and kept inside the room.
struct RoomLayout
{
    //! Left and right.
    std::array<Vec3, 2> sources {};
    //! Per group, left and right.
    std::array<std::array<Mic, 2>, MicGroupCount> mics {};

    static RoomLayout from(const RoomSettings & settings);
};

constexpr double SourceHeightM { 1.5 };
constexpr std::array<double, MicGroupCount> MicHeightM { 1.5, 2.0, 2.5 };

//! The distance a sound is taken to be heard from at full level. A source is no point, and without
//! this a mic on the source would hear it infinitely loud.
constexpr double ReferenceDistanceM { 0.3 };

double volume(const RoomSettings & settings);
double surfaceArea(const RoomSettings & settings);

//! The reverberation time per band, by Eyring, with the air's absorption.
Bands reverbTimes(const RoomSettings & settings);

//! The broadband one: the mean over 500 Hz to 2 kHz.
double midReverbTime(const Bands & reverbTimes);

//! The level of the diffuse tail of a unit impulse, as energy per second, @p seconds after it at
//! the reverberation time @p reverbTime, against the direct sound heard at the reference distance:
//! the density of image sources times the energy each brings.
double diffuseEnergyRate(double seconds, double reverbTime, double volume);

//! The impulse responses from each source to each mic, worked out a slice at a time.
//!
//! The early part only: the direct sound and every reflection, by the image-source method, up to
//! a window that covers the furthest mic's direct sound and the 80 ms after it. Past that the
//! reflections arrive too densely to tell apart, and a feedback network matched to the room's
//! decay takes over (see RoomTail). Each reflection is the image of the source in the walls,
//! weighed by what each wall it bounced off absorbed, what the air took and the mic's pattern in
//! the direction it arrives from, per octave band; the bands are joined into one response by a
//! complementary filter bank.
//!
//! Far too much work for an audio block, so it is done as a job advanced by advance() with a
//! budget of work, counted rather than timed, which keeps a render the same on every run however
//! fast the machine is.
class ImpulseDesigner
{
public:
    static constexpr size_t SourceCount { 2 };
    static constexpr size_t ResponseCount { MicGroupCount * SourceCount * 2 };
    //! The most images of one source a design takes. In a small room the window is shortened so
    //! the images fit, which only hands the tail over earlier.
    static constexpr size_t MaxImages { 6000 };
    static constexpr double MaxWindowSeconds { 0.4 };
    //! The early part's window covers the furthest direct sound and this much after it.
    static constexpr double WindowAfterDirectSeconds { 0.08 };
    //! The early part fades out over the end of its window while the tail fades in.
    static constexpr double CrossoverSeconds { 0.03 };

    //! Sizes everything for @p sampleRate, which allocates.
    void configure(double sampleRate);
    bool isConfigured() const;

    //! Starts a design, abandoning any still running.
    void start(const RoomSettings & settings);
    bool isDesigning() const;
    //! Does up to @p work of the design. Returns true when that finished it.
    bool advance(double work);
    //! Does the rest of the design.
    void finish();

    static size_t responseIndex(MicGroup group, size_t source, size_t mic);
    //! The finished response, length() samples of it.
    const std::vector<double> & response(size_t index) const;
    size_t length() const;
    //! What scales each group's pair, so that each sounds equally loud at an equal level.
    double normalization(MicGroup group) const;
    //! Where the tail takes over and what it decays at.
    double windowSeconds() const;
    double crossoverSeconds() const;
    const Bands & reverbTimes() const;
    const RoomSettings & settings() const;

private:
    struct Image
    {
        Vec3 position;
        //! What the walls it bounced off left of it, as an amplitude, per band.
        Bands reflection {};
    };

    enum class Stage
    {
        Idle,
        Enumerate,
        Deposit,
        Filter,
        Done
    };

    struct Biquad
    {
        double b0 { 1.0 };
        double b1 { 0.0 };
        double b2 { 0.0 };
        double a1 { 0.0 };
        double a2 { 0.0 };
        double z1 { 0.0 };
        double z2 { 0.0 };

        void setLowPass(double hz, double sampleRate);
        double process(double input);
    };

    void prepare();
    void enumerateSlab();
    void depositImages(size_t count);
    void filterSamples(size_t count);
    void finishResponse();
    void computeNormalization();
    void deposit(size_t band, double delaySamples, double value);

    double m_sampleRate { 0.0 };
    size_t m_capacity { 0 };

    RoomSettings m_settings;
    RoomLayout m_layout;
    Bands m_reverbTimes {};
    double m_window { 0.0 };
    double m_crossover { 0.0 };
    size_t m_length { 0 };

    //! Per material slot (walls, floor, ceiling), per band, the amplitude left after n bounces.
    std::array<std::array<std::vector<double>, BandCount>, 3> m_reflectionPowers;
    Bands m_airNepersPerMetre {};

    std::array<std::vector<Image>, SourceCount> m_images;
    std::array<std::vector<double>, BandCount> m_bandBuffers;
    std::array<std::vector<double>, ResponseCount> m_responses;
    std::array<Biquad, BandCount - 1> m_crossovers {};
    std::array<double, ResponseCount> m_energies {};
    std::array<double, MicGroupCount> m_normalization { 1.0, 1.0, 1.0 };

    Stage m_stage { Stage::Idle };
    size_t m_source { 0 };
    int m_slab { 0 };
    int m_slabCount { 0 };
    size_t m_response { 0 };
    size_t m_cursor { 0 };
};

} // namespace noteahead::RoomAcoustics

#endif // ROOM_ACOUSTICS_HPP
