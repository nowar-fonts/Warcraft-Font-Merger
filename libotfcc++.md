# libotfcc++

libotfcc++ is modern C++ font library based on otfcc.

It is currently part of Warcraft Font Merger. C++20 required.

## Design

### JSON as API

```c++
nlohmann::json otfcc::dump_otf(ForwardIterator<byte> begin, ForwardIterator<byte> end);
nlohmann::json otfcc::dump_otf(Iterable<byte> buffer);

vector<byte> otfcc::build_otf(const nlohmann::json &font);
```

### Header-only

```c++
#include <nlohmann/json.hpp>
#include <otfcc/otfcc.hpp>
```

### RAII

No manual memory management.
