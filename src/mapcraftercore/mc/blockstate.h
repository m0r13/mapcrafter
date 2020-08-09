/*
 * Copyright 2012-2018 Moritz Hilscher
 *
 * This file is part of Mapcrafter.
 *
 * Mapcrafter is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mapcrafter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mapcrafter.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BLOCKSTATE_H_
#define BLOCKSTATE_H_

#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace mapcrafter {
namespace mc {

class BlockState {
public:
	BlockState(std::string name = "");

	std::string getName() const;

	const std::map<std::string, std::string>& getProperties() const;
	bool hasProperty(std::string key) const;
	std::string getProperty(std::string key, std::string default_value = "") const;
	void setProperty(std::string key, std::string value);

	const std::string getVariantDescription() const;

	bool operator<(const BlockState& other) const;

	static BlockState parse(std::string name, std::string variant_description);

private:
	void updateVariantDescription();

	std::string name;
	// let's use an ordered map to make sure property-represenation is always the same
	std::map<std::string, std::string> properties;
	// representation of properties that's like: "foo=bar,key1=value,key2=test,"
	std::string variant_description;
};

class BlockStateRegistry {
public:
	BlockStateRegistry();

	uint16_t getBlockID(const BlockState& block);
	const BlockState& getBlockState(uint16_t id) const;

	void addKnownProperty(std::string block, std::string property);
	bool isKnownProperty(std::string block, std::string property) const;

private:
	std::mutex mutex;

	std::map<std::string, std::map<std::string, uint16_t>> block_lookup;
	std::vector<BlockState> block_states;

	std::map<std::string, std::set<std::string>> known_properties;

	BlockState unknown_block;
};

}
}

#endif

