#ifndef ASSIGNMENT_ATLAS_CREATOR_H
#define ASSIGNMENT_ATLAS_CREATOR_H

namespace cg {
template<typename ComponentType>
struct AtlasImage {
    const uint32_t width, height, nChannels;

    AtlasImage(uint32_t width, uint32_t height, uint32_t nChannels)
        : width(width), height(height), nChannels(nChannels),
          _data(new ComponentType[static_cast<size_t>(width) * height * nChannels]) {
    }

    AtlasImage(AtlasImage &&other)
        : width(other.width), height(other.height), nChannels(other.nChannels), _data(other._data) {
        other._data = nullptr;
    }

    AtlasImage(const AtlasImage &) = delete;

    ComponentType *data() {
        return _data;
    }

    const ComponentType *data() const {
        return _data;
    }

    ~AtlasImage() {
        if (data) {
            delete data;
        }
    }

private:
    ComponentType *_data;
};

typename<typename ComponentType, typename...Args>
AtlasImage createAtlas()

}

#endif //ASSIGNMENT_ATLAS_CREATOR_H
