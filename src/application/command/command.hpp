#ifndef COMMAND_HPP
#define COMMAND_HPP

namespace noteahead {

class Command
{
public:
    virtual ~Command() = default;
    virtual void undo() = 0;
    virtual void redo() = 0;
};

} // namespace noteahead

#endif // COMMAND_HPP
