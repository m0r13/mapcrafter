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

#ifndef ISOMETRICNEW_TILERENDERER_H_
#define ISOMETRICNEW_TILERENDERER_H_

#include "../../image.h"
#include "../../tilerenderer.h"

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace mapcrafter {

namespace mc {
class BlockStateRegistry;
}

namespace renderer {

namespace old {

/**
 * Iterates over the top blocks of a tile.
 */
class TileTopBlockIterator {
private:
	int block_size;

	bool is_end;
	int min_row, max_row;
	int min_col, max_col;
	mc::BlockPos top;
public:
	TileTopBlockIterator(const TilePos& tile, int block_size, int tile_width);
	~TileTopBlockIterator();

	void next();
	bool end() const;

	mc::BlockPos current;
	int draw_x, draw_y;
};

/**
 * Iterates over the blocks, which are on a tile on the same position,
 * this means every block is (x+1, z-1 and y-1) of the last block
 */
class BlockRowIterator {
public:
	BlockRowIterator(const mc::BlockPos& block);
	~BlockRowIterator();

	void next();
	bool end() const;

	mc::BlockPos current;
};

/**
 * A block, which should get drawed on a tile.
 */
struct RenderBlock {

	// drawing position in pixels on the tile
	int x, y;
	bool transparent;
	RGBAImage image;
	mc::BlockPos pos;
	uint8_t id, data;

	bool operator<(const RenderBlock& other) const;
};

}

/**
 * Renders tiles from world data.
 */
class NewIsometricTileRenderer : public TileRenderer {
public:
	NewIsometricTileRenderer(const RenderView* render_view, mc::BlockStateRegistry& block_registry,
			BlockImages* images, int tile_width, mc::WorldCache* world, RenderMode* render_mode);
	virtual ~NewIsometricTileRenderer();

	//virtual void renderTile(const TilePos& tile_pos, RGBAImage& tile);

	virtual int getTileSize() const;

protected:
	virtual void renderTopBlocks(const TilePos& tile_pos, std::set<TileImage>& tile_images);
};

}
}

#endif /* ISOMETRICNEW_TILERENDERER_H_ */
