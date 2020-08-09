/*
 * Copyright 2012-2016 Moritz Hilscher
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

#include "worldentities.h"

#include "../util/picojson.h"

namespace mapcrafter {
namespace mc {

/**
 * Checks whether a line from the sign entity is in the new json format (>= mc 1.8).
 * We assume a line is in the new format if it starts and ends with '"', '{'/'}'
 * or if it is 'null'.
 */
bool isJSONLine(const std::string& line) {
	if (line.empty())
		return false;
	return line == "null" || (line[0] == '"' && line[line.size() - 1] == '"')
			|| (line[0] == '{' && line[line.size() - 1] == '}');
}

/**
 * Extracts the sign line text from a json object. Also recursively extracts the json
 * objects in the "extra" array. Throws a std::runtime_error if the json is invalid.
 */
std::string extractTextFromJSON(const picojson::value& value) {
	if (value.is<picojson::null>())
		return "";
	if (value.is<std::string>())
		return value.get<std::string>();
	if (value.is<picojson::object>()) {
		const picojson::object& object = value.get<picojson::object>();
		if (!object.count("text") || !object.at("text").is<std::string>())
			throw std::runtime_error("No string 'text' found");
		std::string extra = "";
		if (object.count("extra")) {
			if (!object.at("extra").is<picojson::array>())
				throw std::runtime_error("Object 'extra' must be an array");
			picojson::array array = object.at("extra").get<picojson::array>();
			for (auto value_it = array.begin(); value_it != array.end(); ++value_it)
				extra += extractTextFromJSON(*value_it);
		}
		return object.at("text").get<std::string>() + extra;
	}
	throw std::runtime_error("Unknown object type");
}

/**
 * Parses a sign line in the json sign line format. Parses the json with the picojson
 * library and uses the extractTextFromJSON function to extract the actual text.
 */
std::string parseJSONLine(const std::string& line) {
	std::string error;
	picojson::value value;
	picojson::parse(value, line.begin(), line.end(), &error);
	if (!error.empty()) {
		LOG(ERROR) << "Unable to parse sign line json '" << line << "': " << error << ".";
		return "";
	}
	try {
		return extractTextFromJSON(value);
	} catch (std::runtime_error& e) {
		LOG(ERROR) << "Invalid json sign line (" << e.what() << "): "  << line;
	}
	return "";
}

SignEntity::SignEntity() {
}

SignEntity::SignEntity(const mc::BlockPos& pos, const Lines& lines)
	: pos(pos), lines(lines), text() {
	// check if the lines of this sign are in the new json format (>= mc 1.8)
	// if yes, extract actual text
	if (isJSONLine(lines[0]) && isJSONLine(lines[1])
			&& isJSONLine(lines[2]) && isJSONLine(lines[3])) {
		this->lines[0] = parseJSONLine(lines[0]);
		this->lines[1] = parseJSONLine(lines[1]);
		this->lines[2] = parseJSONLine(lines[2]);
		this->lines[3] = parseJSONLine(lines[3]);
	}

	// join the lines as sign text
	for (int i = 0; i < 4; i++) {
		std::string line = util::trim(this->lines[i]);
		if (line.empty())
			continue;
		text += line + " ";
	}
	text = util::trim(text);
}

SignEntity::~SignEntity() {
}

const mc::BlockPos& SignEntity::getPos() const {
	return pos;
}

const SignEntity::Lines& SignEntity::getLines() const {
	return lines;
}

const std::string& SignEntity::getText() const {
	return text;
}

WorldEntitiesCache::WorldEntitiesCache(const World& world)
	: world(world), cache_file(world.getRegionDir() / "entities.nbt.gz") {
}

WorldEntitiesCache::~WorldEntitiesCache() {
}

unsigned int WorldEntitiesCache::readCacheFile() {
	if (!fs::exists(cache_file)) {
		LOG(DEBUG) << "Cache file " << cache_file << " does not exist.";
		return 0;
	}

	nbt::NBTFile nbt_file;
	nbt_file.readNBT(cache_file.string().c_str(), nbt::Compression::GZIP);

	nbt::TagList nbt_regions = nbt_file.findTag<nbt::TagList>("regions");
	for (auto region_it = nbt_regions.payload.begin();
			region_it != nbt_regions.payload.end(); ++region_it) {
		nbt::TagCompound region = (*region_it)->cast<nbt::TagCompound>();
		nbt::TagList chunks = region.findTag<nbt::TagList>("chunks");
		mc::RegionPos region_pos;
		region_pos.x = region.findTag<nbt::TagInt>("x").payload;
		region_pos.z = region.findTag<nbt::TagInt>("z").payload;

		for (auto chunk_it = chunks.payload.begin(); chunk_it != chunks.payload.end();
				++chunk_it) {
			nbt::TagCompound chunk = (*chunk_it)->cast<nbt::TagCompound>();
			nbt::TagList entities = chunk.findTag<nbt::TagList>("entities");
			mc::ChunkPos chunk_pos;
			chunk_pos.x = chunk.findTag<nbt::TagInt>("x").payload;
			chunk_pos.z = chunk.findTag<nbt::TagInt>("z").payload;

			for (auto entity_it = entities.payload.begin();
					entity_it != entities.payload.end(); ++entity_it) {
				nbt::TagCompound entity = (*entity_it)->cast<nbt::TagCompound>();
				this->entities[region_pos][chunk_pos].push_back(entity);
			}
		}
	}

	LOG(DEBUG) << "Read cache file " << cache_file << ". Last modification time was "
			<< fs::last_write_time(cache_file) << ".";
	return fs::last_write_time(cache_file);
}

void WorldEntitiesCache::writeCacheFile() const {
	nbt::NBTFile nbt_file;
	nbt::TagList nbt_regions(nbt::TagCompound::TAG_TYPE);

	for (auto region_it = entities.begin(); region_it != entities.end(); ++region_it) {
		nbt::TagCompound nbt_region;
		nbt_region.addTag("x", nbt::TagInt(region_it->first.x));
		nbt_region.addTag("z", nbt::TagInt(region_it->first.z));
		nbt::TagList nbt_chunks(nbt::TagCompound::TAG_TYPE);
		for (auto chunk_it = region_it->second.begin();
				chunk_it != region_it->second.end(); ++chunk_it) {
			nbt::TagCompound nbt_chunk;
			nbt_chunk.addTag("x", nbt::TagInt(chunk_it->first.x));
			nbt_chunk.addTag("z", nbt::TagInt(chunk_it->first.z));
			nbt::TagList nbt_entities(nbt::TagCompound::TAG_TYPE);
			for (auto entity_it = chunk_it->second.begin();
					entity_it != chunk_it->second.end(); ++entity_it) {
				nbt_entities.payload.push_back(nbt::TagPtr(entity_it->clone()));
			}
			nbt_chunk.addTag("entities", nbt_entities);
			nbt_chunks.payload.push_back(nbt::TagPtr(nbt_chunk.clone()));
		}
		nbt_region.addTag("chunks", nbt_chunks);
		nbt_regions.payload.push_back(nbt::TagPtr(nbt_region.clone()));
	}

	nbt_file.addTag("regions", nbt_regions);
	nbt_file.writeNBT(cache_file.string().c_str(), nbt::Compression::GZIP);
}

void WorldEntitiesCache::update(util::IProgressHandler* progress) {
	unsigned int timestamp = readCacheFile();

	auto regions = world.getAvailableRegions();
	if (progress != nullptr)
		progress->setMax(regions.size());
	for (auto region_it = regions.begin(); region_it != regions.end(); ++region_it) {

		fs::path region_path = world.getRegionPath(*region_it);
		if (fs::last_write_time(region_path) < timestamp) {
			LOG(DEBUG) << "Entities of region " << region_path.filename()
					<< " are cached (mtime region " << fs::last_write_time(region_path)
					<< " < mtime cache " << timestamp << ").";
			if (progress != nullptr)
				progress->setValue(progress->getValue() + 1);
			continue;
		} else {
			LOG(DEBUG) << "Entities of region " << region_path.filename()
					<< " are outdated. (mtime region file " << fs::last_write_time(region_path)
					<< " >= mtime cache " << timestamp << "). Updating.";
		}

		RegionFile region;
		world.getRegion(*region_it, region);
		region.read();

		auto chunks = region.getContainingChunks();
		for (auto chunk_it = chunks.begin(); chunk_it != chunks.end(); ++chunk_it) {
			if (region.getChunkTimestamp(*chunk_it) < timestamp)
				continue;

			this->entities[*region_it][*chunk_it].clear();

			mc::nbt::NBTFile nbt;
			const std::vector<uint8_t>& data = region.getChunkData(*chunk_it);
			nbt.readNBT(reinterpret_cast<const char*>(&data[0]), data.size(),
					mc::nbt::Compression::ZLIB);

			nbt::TagCompound& level = nbt.findTag<nbt::TagCompound>("Level");
			if (!level.hasTag<nbt::TagList>("TileEntities")) {
				continue;
			}
			nbt::TagList& entities = level.findTag<nbt::TagList>("TileEntities");
			for (auto entity_it = entities.payload.begin();
					entity_it != entities.payload.end(); ++entity_it) {
				nbt::TagCompound entity = (*entity_it)->cast<nbt::TagCompound>();
				this->entities[*region_it][*chunk_it].push_back(entity);
			}
		}
		if (progress != nullptr)
			progress->setValue(progress->getValue() + 1);
	}

	LOG(DEBUG) << "Writing cache file " << cache_file << " at " << std::time(nullptr) << ".";
	writeCacheFile();
}

std::vector<SignEntity> WorldEntitiesCache::getSigns(WorldCrop world_crop) const {
	std::vector<SignEntity> signs;

	for (auto region_it = entities.begin(); region_it != entities.end(); ++region_it) {
		if (!world_crop.isRegionContained(region_it->first))
			continue;
		for (auto chunk_it = region_it->second.begin();
				chunk_it != region_it->second.end(); ++chunk_it) {
			if (!world_crop.isChunkContained(chunk_it->first))
				continue;
			for (auto entity_it = chunk_it->second.begin();
					entity_it != chunk_it->second.end(); ++entity_it) {
				const nbt::TagCompound& entity = *entity_it;

				if (entity.findTag<nbt::TagString>("id").payload != "Sign"
						&& entity.findTag<nbt::TagString>("id").payload != "minecraft:sign")
					continue;

				mc::BlockPos pos(
					entity.findTag<nbt::TagInt>("x").payload,
					entity.findTag<nbt::TagInt>("z").payload,
					entity.findTag<nbt::TagInt>("y").payload
				);

				if (!world_crop.isBlockContainedXZ(pos)
						|| !world_crop.isBlockContainedY(pos))
					continue;

				mc::SignEntity::Lines lines = {{
					entity.findTag<nbt::TagString>("Text1").payload,
					entity.findTag<nbt::TagString>("Text2").payload,
					entity.findTag<nbt::TagString>("Text3").payload,
					entity.findTag<nbt::TagString>("Text4").payload
				}};

				signs.push_back(mc::SignEntity(pos, lines));
			}
		}
	}

	return signs;
}

} /* namespace mc */
} /* namespace mapcrafter */
