#ifndef UTILS_H
#define UTILS_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <vector>
#include <godot_cpp/variant/transform3d.hpp>

namespace godot
{

class Utils : public Object
{
    GDCLASS(Utils, Object);

  public:
    Utils()
    {
    }
    ~Utils()
    {
    }
    static PackedFloat32Array vector_to_array_float(const std::vector<float> &vec)
    {
        PackedFloat32Array array;
        array.resize(vec.size());
        std::copy(vec.begin(), vec.end(), array.ptrw());
        return array;
    }

    static std::vector<float> array_to_vector_float(const PackedFloat32Array &array)
    {
        std::vector<float> vec(array.size());
        std::copy(array.ptr(), array.ptr() + array.size(), vec.begin());
        return vec;
    }

    static PackedInt32Array vector_to_array_int(const std::vector<int> &vec)
    {
        PackedInt32Array array;
        array.resize(vec.size());
        std::copy(vec.begin(), vec.end(), array.ptrw());
        return array;
    }

    static std::vector<int> array_to_vector_int(const PackedInt32Array &array)
    {
        std::vector<int> vec(array.size());
        std::copy(array.ptr(), array.ptr() + array.size(), vec.begin());
        return vec;
    }

    static godot::String vector_to_string_float(const std::vector<float> &vec)
    {
        godot::String string = "[";
        for (const float &f : vec)
        {
            string += String::num(f, 2) + ", ";
        }
        return string + "]";
    }

    // template <class matType> static godot::String to_string(const matType &v)
    // {
    //     return godot::String(("{" + glm::to_string(v) + "}").c_str());
    // }

    static void print_projection(Projection projection)
    {
        String str = "Projection:\n";
        for (int i = 0; i < 4; i++)
        {
            str += String::num(projection[i].x) + ", " + String::num(projection[i].y) + ", " +
                   String::num(projection[i].z) + ", " + String::num(projection[i].w) + "\n";
        }
        UtilityFunctions::print(str);
    }

    static inline void projection_to_float(float *target, const Projection &t)
    {
        for (size_t i = 0; i < 4; i++)
        {
            target[i * 4] = t.columns[i].x;
            target[i * 4 + 1] = t.columns[i].y;
            target[i * 4 + 2] = t.columns[i].z;
            target[i * 4 + 3] = t.columns[i].w;
        }
    }

    static inline void projection_to_float_transposed(float *target, const Projection &t)
    {
        for (size_t i = 0; i < 4; i++)
        {
            target[i * 4] = t.columns[0][i];
            target[i * 4 + 1] = t.columns[1][i];
            target[i * 4 + 2] = t.columns[2][i];
            target[i * 4 + 3] = t.columns[3][i];
        }
    }

    static inline void transform_to_float(float *target, const Transform3D &t)
    {
        // Pack as column-major 4x4 (matches GLSL mat4 default)
        const Basis &b = t.get_basis();
        const Vector3 &x = b.get_column(0);
        const Vector3 &y = b.get_column(1);
        const Vector3 &z = b.get_column(2);
        const Vector3 &o = t.get_origin();

        // Column 0 (X axis)
        target[0] = x.x; target[1] = x.y; target[2] = x.z; target[3] = 0.0f;
        // Column 1 (Y axis)
        target[4] = y.x; target[5] = y.y; target[6] = y.z; target[7] = 0.0f;
        // Column 2 (Z axis)
        target[8] = z.x; target[9] = z.y; target[10] = z.z; target[11] = 0.0f;
        // Column 3 (Origin)
        target[12] = o.x; target[13] = o.y; target[14] = o.z; target[15] = 1.0f;
    }

    static inline unsigned int compress_color16(Color rgb)
    {

        // H: 7 bits, S: 4 bits, V: 5 bits
        unsigned int h = static_cast<unsigned int>(rgb.get_h() * 127.0);
        unsigned int s = static_cast<unsigned int>(rgb.get_s() * 15.0);
        unsigned int v = static_cast<unsigned int>(rgb.get_v() * 31.0);

        // Pack into a single unsigned int
        return (h << 9) | (s << 5) | v;
    }

    static inline Color decompress_color16(unsigned int packedColor)
    {
        // Extract H, S, V components
        unsigned int h = (packedColor >> 9) & 0x7F; // 7 bits for hue
        unsigned int s = (packedColor >> 5) & 0x0F; // 4 bits for saturation
        unsigned int v = packedColor & 0x1F;        // 5 bits for value

        // Convert back to RGB
        return Color::from_hsv(float(h) / 128.0, float(s) / 16.0, float(v) / 32.0);
    }

    static Ref<RandomNumberGenerator> rng;

    static Color randomized_color(Color color)
    {
        Vector3 hsv = {color.get_h(), color.get_s(), color.get_v()};
        // Math::randf
        if (!rng.is_valid())
        {
            rng.instantiate();
            rng->set_seed(Time::get_singleton()->get_unix_time_from_system());
        }

        hsv.x = Math::clamp(hsv.x + rng->randf() * 0.025f, 0.0f, 1.0f);
        hsv.y = Math::clamp(hsv.y *(0.95f + rng->randf() * 0.1f), 0.0f, 1.0f);
        hsv.z = Math::clamp(hsv.z *(0.95f + rng->randf() * 0.1f), 0.0f, 1.0f);

        return Color::from_hsv(hsv.x, hsv.y, hsv.z);
    }
};

} // namespace godot

#endif
