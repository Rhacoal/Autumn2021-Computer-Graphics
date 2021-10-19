#include <obj_loader.h>

#include <fstream>
#include <vector>
#include <regex>
#include <optional>

static std::vector<std::string> split(const std::string &str, const std::regex &delim) {
    // https://stackoverflow.com/a/45204031/14470738
    return std::vector<std::string>{std::sregex_token_iterator{str.begin(), str.end(), delim, -1}, {}};
}

cg::Mesh *cg::read(const std::filesystem::path &path) {
    using vertex = std::tuple<float, float, float>;
    using texCoord = std::tuple<float, float, float>;
    using normal = std::tuple<float, float, float>;
    using face_vert = std::tuple<int, std::optional<int>, std::optional<int>>;
    using face = std::tuple<face_vert, face_vert, face_vert, std::optional<face_vert>>;
    struct group {
        std::string material;
        std::vector<face> faces;
    };
    std::ifstream fin(path, std::ios_base::in | std::ios_base::binary);
    std::vector<vertex> vertices(1);
    std::vector<texCoord> texCoords(1);
    std::vector<normal> normals(1);
    std::vector<group> groups;

    std::string line;
    std::regex blank(R"(\s+)");
    while (std::getline(fin, line)) {
        auto cmd = split(line, blank);
        if (cmd[0] == "v") {
            if (cmd.size() != 4) {
                fprintf(stderr, "unexpected sequence: %s\n", line.c_str());
                return nullptr;
            }
            float x = strtof(cmd[1].c_str(), nullptr);
            float y = strtof(cmd[2].c_str(), nullptr);
            float z = strtof(cmd[3].c_str(), nullptr);
            vertices.emplace_back(x, y, z);
        } else if (cmd[0] == "vt") {
            if (cmd.size() != 2 && cmd.size() != 3) {
                fprintf(stderr, "unexpected sequence: %s\n", line.c_str());
                return nullptr;
            }
            float u = strtof(cmd[1].c_str(), nullptr);
            float v = strtof(cmd[2].c_str(), nullptr);
            float w = cmd.size() == 3.0 ? strtof(cmd[3].c_str(), nullptr) : 0.0;
            texCoords.emplace_back(u, v, w);
        } else if (cmd[0] == "vn") {
            if (cmd.size() != 4) {
                fprintf(stderr, "unexpected sequence: %s\n", line.c_str());
                return nullptr;
            }
            float x = strtof(cmd[1].c_str(), nullptr);
            float y = strtof(cmd[2].c_str(), nullptr);
            float z = strtof(cmd[3].c_str(), nullptr);
            normals.emplace_back(x, y, z);
        } else if (cmd[0] == "f") {
            if (!groups.size()) {
                fprintf(stderr, "unexpected f before g\n");
            }
            if (cmd.size() != 4 && cmd.size() != 5) {
                fprintf(stderr, "only supports triangles and rectangles: %s\n", line.c_str());
            }
        }
    }
    // TODO
    return nullptr;
}
