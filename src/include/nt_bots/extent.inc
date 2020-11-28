#if !defined(_NT_BOTS_EXTENT_INC_)
#define _NT_BOTS_EXTENT_INC_

enum struct Extent {
	float lo[3];
	float hi[3];
	
	float SizeX() { return this.hi[0] - this.lo[0]; }
	float SizeY() { return this.hi[1] - this.lo[1]; }
	float SizeZ() { return this.hi[2] - this.lo[2]; }
	
	// return true if 'pos' is inside of this extent
	bool Contains(const float[3] pos)
	{
		return (pos[0] >= this.lo[0] && pos[0] <= this.hi[0] &&
			pos[1] >= this.lo[1] && pos[1] <= this.hi[1] &&
			pos[2] >= this.lo[2] && pos[2] <= this.hi[2]);
	}
	
	// return true if this extent overlaps the given one
	bool IsOverlapping(const float[3] lo, const float[3] hi)
	{
		return (this.lo[0] <= hi[0] && this.hi[0] >= lo[0] &&
			this.lo[1] <= hi[1] && this.hi[1] >= lo[1] &&
			this.lo[2] <= hi[2] && this.hi[2] >= lo[2]);
	}
	
	// return true if this extent completely contains the given one
	bool IsEncompassing(const float[3] lo, const float[3] hi, const float tolerance = 0.0)
	{
		return (this.lo[0] <= lo[0] + tolerance && this.hi[0] >= hi[0] - tolerance &&
			this.lo[1] <= lo[1] + tolerance && this.hi[1] >= hi[1] - tolerance &&
			this.lo[2] <= lo[2] + tolerance && this.hi[2] >= hi[2] - tolerance);
	}
}

#endif // _NT_BOTS_EXTENT_INC_