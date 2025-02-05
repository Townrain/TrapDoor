//
// Created by xhy on 2020/10/23.
//

#include "CommandNode.h"

#include <algorithm>
#include <utility>

#include "tools/Message.h"
#include "tools/MsgBuilder.h"

namespace trapdoor {
    namespace {
        bool isValidIntString(const std::string &str) {
            return std::all_of(str.begin(), str.end(), [](char c) {
                return ('0' <= c && c <= '9') || c == '-';
            });
        }
    }  // namespace

    CommandNode *Arg(const std::string &args, const std::string &desc,
                     ArgType type) {
        auto *node = new CommandNode(args, desc);
        node->setArgType(type);
        return node;
    }

    CommandNode::CommandNode(std::string name, std::string description)
        : name(std::move(name)),
          work([this](ArgHolder *holder, Actor *actor) {
              error(actor, LANG("command.error.moreTokenRequired"),
                    this->name.c_str());
          }),
          description(std::move(description)) {}

    CommandNode::CommandNode(std::string name)
        : CommandNode(std::move(name), LANG("command.error.noDesc")) {}

    int CommandNode::parse(Actor *player,
                           const std::vector<std::string> &tokens, size_t idx) {
        bool executeNow = false;
        if (idx == tokens.size()) {  //当前位置已经没tokens了
            executeNow = true;
            if (this->argType == ArgType::NONE) {
                ArgHolder holder(0);
                this->run(&holder, player);
            } else {
                error(player, LANG("command.error.moreTokenRequired"),
                      tokens[idx - 1].c_str());
            }
        } else if (idx == tokens.size() - 1) {  //当前是最后一个token
            ArgHolder *holder = nullptr;
            switch (this->argType) {
                case ArgType::INT:
                    executeNow = isValidIntString(tokens[idx]);
                    holder =
                        integerArg(strtol(tokens[idx].c_str(), nullptr, 10));
                    break;
                case ArgType::BOOL:
                    executeNow = tokens[idx] == "true" ||
                                 tokens[idx] == "false" || tokens[idx] == "1" ||
                                 tokens[idx] == "0";
                    holder =
                        boolArg(tokens[idx] == "true" || tokens[idx] == "1");
                    break;
                case ArgType::STR:
                    executeNow = true;
                    holder = strArg(tokens[idx]);
                    break;
                case ArgType::NONE:
                    executeNow = false;
                    break;
            }
            if (executeNow) {
                this->run(holder, player);
                delete holder;
            }
        }
        if (!executeNow) {
            for (const auto &node : this->nextNodes) {
                if (node.second->getName() == tokens[idx]) {
                    return node.second->parse(player, tokens, idx + 1);
                }
            }
            std::string s = LANG("command.error.noSubCommand");
            for (auto &node : this->nextNodes) {
                s += "[";
                s += node.first;
                s += "] ";
            }
            error(player, "%s", s.c_str());
        }
        return 0;
    }

    void CommandNode::getHelpInfo(int idx, std::string &buffer) const {
        if (this->getName() == "?") return;
        MessageBuilder builder;
        if (idx != 0) {
            builder += std::string(idx * 4, ' ');
        }

        builder.sText(this->getName(), MSG_COLOR::YELLOW | MSG_COLOR::BOLD);
        builder += " [";
        switch (this->argType) {
            case ArgType::NONE:
                builder += "none";
                break;
            case ArgType::INT:
                builder += "integer";
                break;
            case ArgType::BOOL:
                builder += "true/false";
                break;
            case ArgType::STR:
                builder += "string";
                break;
        }
        builder += "] - ";
        builder += this->getDescription();
        builder += "\n";
        buffer += builder.get();
        for (auto &node : this->nextNodes) {
            node.second->getHelpInfo(idx + 1, buffer);
        }
    }

    CommandNode *CommandNode::then(CommandNode *node) {
        this->nextNodes[node->getName()] = node;
        return this;
    }

    std::string CommandNode::getDescription() const {
        return LANG(this->description);
    }

    void CommandNode::run(ArgHolder *holder, Actor *player) {
        if (!player) return;
        this->work(holder, player);
    }

    const char *commandPermissionLevelToStr(CommandPermissionLevel level) {
        switch (level) {
            case Any:
                return "any";
            case GameMasters:
                return "GameMaster";
            case Admin:
                return "Admin";
            case Host:
                return "host";
            case Owner:
                return "Owner";
            case Internal:
                return "Internal";
            default:
                return "Unknown";
        }
    }

}  // namespace trapdoor