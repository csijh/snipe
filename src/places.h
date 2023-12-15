// Snipe editor. Free and open source, see licence.txt.

// A gap buffer of places in the source text. Can be used to track line
// boundaries or brackets, for example. The places before the gap are normal
// indexes. The places after the gap are relative to the end of the text, so
// that they remain stable across insertions and deletions at the cursor. Text
// insertions and deletions are monitored, to track the size of the text, and
// make any necessary but usually minor adjustments to the gap position.
typedef struct places Places;

// Create or free a places object.
Places *newPlaces();
void freePlaces(Places *ps);

// The total number of places (either side of the gap).
int sizeP(Places *ps);

// Report an insertion of n bytes (n > 0) or deletion (n < 0) at place p.
// Remove any places that are inside the deletion, or a duplicate place.
void editP(Places *ps, int p, int n);

// Get the i'th place (non-negative).
int getP(Places *ps);

// Set the i'th Place.
int setP(Places *ps);

// Insert a place at position i.
void insertP(Places *ps, int i, int p);

// Delete the place at position i.
void deleteP(Places *ps, int i);
