#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <set>

#include "raylib.h"

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "vec_ops.h"

#define DECLARE_PTR_TYPE(type) public: using Ptr = std::shared_ptr<type>;

typedef uint32_t u32;
typedef uint8_t u8;

typedef Eigen::Vector3i v3i;
typedef Eigen::Vector2i vi;
typedef Eigen::Vector2f v2f;
typedef Eigen::Vector3f v3f;
typedef Eigen::Matrix3f m3f;
typedef Eigen::Matrix4f m4f;
typedef Eigen::Quaternionf quat;
    
inline std::string vec2str(const v3f& vec) {
    std::stringstream str;
    str << "[";
    for (int i = 0; i < 3; i++)
    {
        str << vec(i);
        if (i < 2)
            str << ", ";
        else
            str << "]" << std::endl;
    }
    str << std::endl;
    return str.str();
}

inline std::string vec2strCSV(const v3f& vec) {
    std::stringstream str;
    for (int i = 0; i < 3; i++)
    {
        str << vec(i);
        if (i < 2)
            str << " ";
    }
    return str.str();
}

inline std::string mat2str(const m4f& mat) {
    std::stringstream str;
    for (int i = 0; i < 4; i++)
    {
        str << "- [";
        for (int j = 0; j < 4; j++)
        {
            str << mat(i, j);
            if (j < 3)
                str << ", ";
            else
                str << "]" << std::endl;
        }
    }
    str << std::endl;
    return str.str();
}

inline void printMat(const m4f& mat) {
    std::cout << mat2str(mat);
}

inline void printVec(const v3f& vec) {
    std::cout << vec2str(vec);
}

struct Pose {
    v3f  pos = v3f::Zero();
    quat rot = quat::Identity();

    m4f matrix() const {
        m4f res = m4f::Identity();
        res.block<3, 3>(0, 0) = rot.matrix();
        res.block<3, 1>(0, 3) = pos;
        return res;
    }   

    static Pose fromMatrix(const m4f& mat) {
        Pose res;
        res.rot = quat(mat.block<3, 3>(0, 0));
        res.pos = mat.block<3, 1>(0, 3);
        return res;
    }

    void print() {
        printMat(matrix());
    }

    Pose() {
        pos = v3f::Zero();
        rot = quat::Identity();
    }
    
    Pose(const v3f& pos, const quat& rot = quat::Identity()) : 
        pos(pos), rot(rot) 
    { }

    Pose(const m4f& mat) {
        Pose pose = fromMatrix(mat);
        pos = pose.pos;
        rot = pose.rot;
    }

    Pose& operator=(const m4f& mat) {
        Pose pose = fromMatrix(mat);
        *this = pose;
        return *this;
    }

    Pose operator*(const Pose& p) {
        return fromMatrix(this->matrix() * p.matrix());
    }

    Pose inverse() {
        return Pose::fromMatrix(this->matrix().inverse());
    }

    DECLARE_PTR_TYPE(Pose);
};   