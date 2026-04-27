#include "src/domain/song.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QXmlStreamReader>
#include <iostream>

using namespace noteahead;

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename.nahd>" << std::endl;
        return 1;
    }

    QFile file(argv[1]);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "Could not open file" << std::endl;
        return 1;
    }

    QXmlStreamReader reader(&file);
    auto song = std::make_unique<Song>();

    try {
        while (!reader.atEnd()) {
            if (reader.isStartElement() && reader.name() == "Project") {
                // We need to skip Project and call Song::deserializeFromXml when we hit Song
                while (!reader.atEnd()) {
                    reader.readNext();
                    if (reader.isStartElement() && reader.name() == "Song") {
                        song->deserializeFromXml(reader, nullptr, nullptr, nullptr, nullptr);
                        break;
                    }
                }
            }
            reader.readNext();
        }

        if (reader.hasError()) {
            std::cerr << "XML Error: " << reader.errorString().toStdString() << " at line " << reader.lineNumber() << std::endl;
            return 1;
        }

        std::cout << "Song loaded successfully!" << std::endl;
        std::cout << "BPM: " << song->beatsPerMinute() << std::endl;
        std::cout << "Patterns: " << song->patternCount() << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception" << std::endl;
        return 1;
    }

    return 0;
}
