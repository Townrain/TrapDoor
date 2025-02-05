//
// Created by xhy on 2020/8/25.
//
#include "Particle.h"

#include <string>

#include "BDSMod.h"
#include "entity/Actor.h"
#include "lib/SymHook.h"
#include "lib/mod.h"
#include "tools/DirtyLogger.h"
#include "world/Level.h"

namespace trapdoor {
    using namespace SymHook;
    namespace {
        //把整数进行二进制分割，获取粒子的生成坐标
        std::map<float, int> binSplit(float start, float end) {
            std::map<float, int> lengthMap;
            int length = static_cast<int>(end - start);
            while (length >= 512) {
                length -= 256;
                auto point = static_cast<float>(128.0 + start);
                start += 256.0;
                lengthMap.insert({point, 256});
            }

            for (auto defaultLength = 256; defaultLength >= 1;
                 defaultLength /= 2) {
                if (length >= defaultLength) {
                    length -= defaultLength;
                    auto point =
                        static_cast<float>(0.5 * defaultLength + start);
                    start += defaultLength;
                    lengthMap.insert({point, defaultLength});
                }
            }
            return lengthMap;
        }

        std::string getLineParticleType(int length, FACING direction,
                                        GRAPHIC_COLOR color) {
            std::string str = "trapdoor:line";
            str += std::to_string(length);
            switch (direction) {
                case FACING::NEG_Y:
                    str += "Yp";
                    break;
                case FACING::POS_Y:
                    str += "Ym";
                    break;
                case FACING::NEG_Z:
                    str += "Zp";
                    break;
                case FACING::POS_Z:
                    str += "Zm";
                    break;
                case FACING::NEG_X:
                    str += "Xp";
                    break;
                case FACING::POS_X:
                    str += "Xm";
                    break;
            }

            switch (color) {
                case GRAPHIC_COLOR::WHITE:
                    str += "W";
                    break;
                case GRAPHIC_COLOR::RED:
                    str += "R";
                    break;
                case GRAPHIC_COLOR::YELLOW:
                    str += "Y";
                    break;
                case GRAPHIC_COLOR::BLUE:
                    str += "B";
                    break;
                case GRAPHIC_COLOR::GREEN:
                    str += "G";
                    break;
            }
            return str;
        }
    }  // namespace

    void drawLine(const Vec3 &originPoint, FACING direction, float length,
                  GRAPHIC_COLOR color, int dimType) {
        if (length <= 0) return;
        float start = 0, end = 0;
        switch (direction) {
            case FACING::NEG_Y:
                start = originPoint.y - length;
                end = originPoint.y;
                break;
            case FACING::POS_Y:
                start = originPoint.y;
                end = originPoint.y + length;
                break;
            case FACING::NEG_Z:
                start = originPoint.z - length;
                end = originPoint.z;
                break;
            case FACING::POS_Z:
                start = originPoint.z;
                end = originPoint.z + length;
                break;
            case FACING::NEG_X:
                start = originPoint.x - length;
                end = originPoint.x;
                break;
            case FACING::POS_X:
                start = originPoint.x;
                end = originPoint.x + length;
                break;
        }
        // split point list;
        auto list = binSplit(start, end);
        std::map<Vec3, int> positionList;
        if (facingIsX(direction)) {
            for (auto i : list)
                positionList.insert(
                    {{i.first, originPoint.y, originPoint.z}, i.second});
        } else if (facingIsY(direction)) {
            for (auto i : list)
                positionList.insert(
                    {{originPoint.x, i.first, originPoint.z}, i.second});
        } else if (facingIsZ(direction)) {
            for (auto i : list)
                positionList.insert(
                    {{originPoint.x, originPoint.y, i.first}, i.second});
        }

        for (auto points : positionList) {
            auto particleType =
                getLineParticleType(points.second, direction, color);
            auto particleTypeInv =
                getLineParticleType(points.second, invFacing(direction), color);
            spawnParticle(points.first, particleType, dimType);
            if (!bdsMod->getCfg().particlePerformanceMode)
                spawnParticle(points.first, particleTypeInv, dimType);
        }
    }

    void drawPoint(const Vec3 &point, GRAPHIC_COLOR color, int dimType) {
        std::string particleType = "trapdoor:point";
        // std::string particleTypeBack = "trapdoor:point_back";
        switch (color) {
            case GRAPHIC_COLOR::WHITE:
                particleType += "W";
                // particleTypeBack += "W";
                break;
            case GRAPHIC_COLOR::RED:
                particleType += "R";
                // particleTypeBack += "R";
                break;
            case GRAPHIC_COLOR::YELLOW:
                particleType += "Y";
                // particleTypeBack += "Y";
                break;
            case GRAPHIC_COLOR::BLUE:
                particleType += "B";
                // particleTypeBack += "B";
                break;
            case GRAPHIC_COLOR::GREEN:
                particleType += "G";
                // particleTypeBack += "G";
                break;
        }
        spawnParticle(point, particleType, dimType);
        // spawnParticle(point, particleTypeBack, dimType);
    }

    void spawnParticle(Vec3 p, std::string &type, int dimType) {
        auto pos = p.toBlockPos();
        auto level = trapdoor::bdsMod->getLevel();
        if (!level) {
            L_ERROR("level is null");
            return;
        }
        auto player = level->getNearestDimensionPlayer(pos, dimType);
        if (!player) {
            return;
        }
        auto maxDist = trapdoor::bdsMod->getCfg().particleViewDistance;
        if (p.distanceTo(*player->getPos()) > static_cast<float>(maxDist))
            return;

        SYM_CALL(void (*)(Level *, const std::string &, Vec3 *, void *),
                 Level_spawnParticleEffect_52e7de09, level, type, &p,
                 player->getDimension());
    }

    void spawnRectangleParticle(const AABB &aabb, GRAPHIC_COLOR color,
                                bool mark, int dimType) {
        auto p1 = aabb.p1, p2 = aabb.p2;
        auto dx = p2.x - p1.x;
        auto dy = p2.y - p1.y;
        auto dz = p2.z - p1.z;
        drawLine(p1, FACING::POS_X, dx, color, dimType);
        if (mark) {
            drawLine(p1, FACING::POS_Y, dy, GRAPH_COLOR::WHITE, dimType);
        } else {
            drawLine(p1, FACING::POS_Y, dy, color, dimType);
        }
        drawLine(p1, FACING::POS_Z, dz, color, dimType);

        Vec3 p3{p2.x, p1.y, p2.z};
        drawLine(p3, FACING::NEG_X, dx, color, dimType);
        drawLine(p3, FACING::POS_Y, dy, color, dimType);
        drawLine(p3, FACING::NEG_Z, dz, color, dimType);
        Vec3 p4{p2.x, p2.y, p1.z};
        drawLine(p4, FACING::NEG_X, dx, color, dimType);
        drawLine(p4, FACING::NEG_Y, dy, color, dimType);
        drawLine(p4, FACING::POS_Z, dz, color, dimType);

        Vec3 p5{p1.x, p2.y, p2.z};
        drawLine(p5, FACING::POS_X, dx, color, dimType);
        drawLine(p5, FACING::NEG_Y, dy, color, dimType);
        drawLine(p5, FACING::NEG_Z, dz, color, dimType);
    }

    void spawnLineParticle(const Vec3 &p, FACING facing, float length,
                           GRAPHIC_COLOR color, int dimType) {
        drawLine(p, facing, length, color, dimType);
    }

    void spawnChunkSurfaceParticle(const ChunkPos &p, int dimID) {
        bool isSlime = p.isSlimeChunk();
        std::string pName2 =
            isSlime ? "trapdoor:chunkslimep" : "trapdoor:chunkp";
        std::string pName1 =
            isSlime ? "trapdoor:chunkslimem" : "trapdoor:chunkm";
        float x = static_cast<float>(p.x) * 16.0f;
        float z = static_cast<float>(p.z) * 16.0f;
        Vec3 p1{x + 0.01f, 128.0f, z + 8.0f};
        Vec3 p2{x + 15.99f, 128.0f, z + 8.0f};
        Vec3 p3{x + 8.0f, 128.0f, z + 0.01f};
        Vec3 p4{x + 8.0f, 128.0f, z + 15.99f};
        spawnParticle(p1, pName1, dimID);
        spawnParticle(p2, pName1, dimID);
        spawnParticle(p3, pName2, dimID);
        spawnParticle(p4, pName2, dimID);
    }

    void spawnSlimeChunkParticle(const ChunkPos &p) {
        float x = static_cast<float>(p.x) * 16.0f;
        float z = static_cast<float>(p.z) * 16.0f;
        Vec3 p1{x + 0.01f, 0.0f, z + 8.0f};
        Vec3 p2{x + 15.99f, 0.0f, z + 8.0f};
        Vec3 p3{x + 8.0f, 0.0f, z + 0.01f};
        Vec3 p4{x + 8.0f, 0.0f, z + 15.99f};
        Vec3 top{x + 8.0f, 128.0f, z + 8.0f};
        std::string pName1 = "trapdoor:slime_side1";
        std::string pName2 = "trapdoor:slime_side2";
        std::string pName3 = "trapdoor:slime_top";
        spawnParticle(p1, pName1);
        spawnParticle(p2, pName1);
        spawnParticle(p3, pName2);
        spawnParticle(p4, pName2);
        spawnParticle(top, pName3);
    }
}  // namespace trapdoor