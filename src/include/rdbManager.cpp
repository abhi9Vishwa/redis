#include "rdbManager.hpp"

#include <bits/stdc++.h>

#include "helperFunc.hpp"
using namespace std;

std::string getRdbDirectory(RedisInfo& redisInfo) {
	std::string dir = redisInfo.getRdbDir();
	return RESPBulkStringEncoder(dir);
}

std::string getRdbFilename(RedisInfo& redisInfo) {
	std::string file = redisInfo.getRdbFilename();
	return RESPBulkStringEncoder(file);
}

void encodeExpiry(vector<uint8_t>& res, long long exp) {
	res.push_back(0xFC);
	uint64_t v = exp;
	for(int i = 0; i < 8; i++)
		res.push_back((v >> (8 * i)) & 0xFF);  // little endian
}

void encodeKeyVal(vector<uint8_t>& res, string& key, string& val) {
	res.push_back(0x00);
	// Encode key first
	auto kt = encodeString(key);
	res.insert(res.end(), kt.begin(), kt.end());

	// Try integer encodings
	try {
		long long vInt = stoll(val);      // signed
		if(vInt >= INT8_MIN && vInt <= INT8_MAX) {
			// C0 = int8
			res.push_back(0xC0);
			res.push_back((uint8_t)(vInt & 0xFF));
			return;
		}
		if(vInt >= INT16_MIN && vInt <= INT16_MAX) {
			// C1 = int16 (little-endian)
			res.push_back(0xC1);
			res.push_back((uint8_t)(vInt & 0xFF));
			res.push_back((uint8_t)((vInt >> 8) & 0xFF));
			return;
		}
		if(vInt >= INT32_MIN && vInt <= INT32_MAX) {
			// C2 = int32 (little-endian)
			res.push_back(0xC2);
			for(int i = 0; i < 4; i++)
				res.push_back((uint8_t)((vInt >> (8 * i)) & 0xFF));
			return;
		}
	}
	catch(...) {
		// Not an integer (fall through to normal string encoding)
	}

	// Fallback: encode as string
	auto vt = encodeString(val);
	res.insert(res.end(), vt.begin(), vt.end());
}


vector<uint8_t> rdbInsertKeyValue(RedisAllData& redisDb) {
	vector<uint8_t> res;
	lock_guard lock(redisDb.store_mtx);

	for(auto i : redisDb.store) {
		string key = i.first;
		string val = i.second;

		std::cout << "k :" << key << "v: " << val << endl;

		if(redisDb.expiry.find(key) != redisDb.expiry.end()) {
			long long expiryTS = redisDb.expiry[key];
			encodeExpiry(res, expiryTS);
		}

		encodeKeyVal(res, key, val);
	}
	return res;
}

string saveRdbDump(RedisAllData& redisDb, RedisInfo& redisInfo) {
	vector<uint8_t> rdb;
	// header sec
	string header = "REDIS0011";
	vector<uint8_t> temp = convertStrToByteVec(header);
	rdb.insert(rdb.end(), temp.begin(), temp.end());

	//metadata
	rdb.push_back(0xFA);
	string metadata = "redis-ver";
	string metadataAttr = "0.1.3";
	temp = encodeString(metadata);
	rdb.insert(rdb.end(), temp.begin(), temp.end());
	temp = encodeString(metadataAttr);
	rdb.insert(rdb.end(), temp.begin(), temp.end());

	// Begin database 
	rdb.push_back(0xFE);
	rdb.push_back(0x00);

	// hash table section
	rdb.push_back(0xFB);
	std::cout << "store size()" << redisDb.store.size();
	std::cout << "expiry size()" << redisDb.expiry.size();
	{
		lock_guard lock(redisDb.store_mtx);
		temp = encodeLength(redisDb.store.size());
		rdb.insert(rdb.end(), temp.begin(), temp.end());
		temp = encodeLength(redisDb.expiry.size());
		rdb.insert(rdb.end(), temp.begin(), temp.end());

	}
	temp = rdbInsertKeyValue(redisDb);
	std::cout << "keyval size " << temp.size() << endl;
	rdb.insert(rdb.end(), temp.begin(), temp.end());
	rdb.push_back(0xFF);

	// uint64_t crc = crc64(0, rdb.data(), rdb.size());
	uint64_t crc = 0x893bb74ef80f7719;
	for(int i = 0; i < 8; i++) {
		rdb.push_back((crc >> (56 - i * 8)) & 0xFF);
	}

	string filePath = redisInfo.getRdbDir() + "/" + redisInfo.getRdbFilename();

	ofstream out(filePath, ios::binary);
	if(!out.is_open()) cerr << "unable to open dump file";
	out.write((char*)rdb.data(), rdb.size());
	out.close();

	std::cout << "Saved RDB at " << filePath << endl;
	return "+OK\r\n";
}

uint8_t read8(ifstream& in) {
    char c;
    in.read(&c, 1);
    return (uint8_t)c;
}

uint32_t read32le(ifstream& in) {
    uint32_t v = 0;
    for (int i = 0; i < 4; i++)
        v |= (uint32_t)read8(in) << (8 * i);
    return v;
}

uint64_t read64le(ifstream& in) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++)
        v |= (uint64_t)read8(in) << (8 * i);
    return v;
}

uint64_t decodeLength(ifstream& in, bool& isEncoded) {
    uint8_t first = read8(in);
    uint8_t flag = (first & 0xC0) >> 6; // top 2 bits

    if (flag == 0) {
        isEncoded = false;
        return first & 0x3F;
    }
    else if (flag == 1) {
        isEncoded = false;
        uint8_t second = read8(in);
        return ((first & 0x3F) << 8) | second;
    }
    else if (flag == 2) {
        isEncoded = false;
        uint32_t b1 = read8(in);
        uint32_t b2 = read8(in);
        uint32_t b3 = read8(in);
        uint32_t b4 = read8(in);
        return (uint64_t)(b1 << 24 | b2 << 16 | b3 << 8 | b4);
    }
    else {
        // Encoded string (integers: C0,C1,C2)
        isEncoded = true;
        return first & 0x3F;
    }
}

string decodeString(ifstream& in) {
    bool isEncoded = false;
    uint64_t len = decodeLength(in, isEncoded);

    if (!isEncoded) {
        string s(len, '\0');
        in.read(&s[0], len);
        return s;
    }

    // Encoded integer
    if (len == 0) {  // C0 = int8
        int8_t v = read8(in);
        return to_string(v);
    }
    if (len == 1) {  // C1 = int16
        int16_t v = read8(in) | (read8(in) << 8);
        return to_string(v);
    }
    if (len == 2) {  // C2 = int32
        int32_t v = 0;
        for (int i = 0; i < 4; i++)
            v |= read8(in) << (8 * i);
        return to_string(v);
    }

    throw runtime_error("Unknown encoding in string");
}

string loadRDB(string& file, RedisAllData& redisDb) {
	vector<string> res;
    ifstream in(file, ios::binary);
    if (!in.is_open()) {
        string cerr =  "Failed to open RDB";
        return cerr;
    }

    // 1. HEADER
    char header[9];
    in.read(header, 9);
    if (strncmp(header, "REDIS0011", 9) != 0) {
        throw runtime_error("Not a valid Redis RDB 11 file");
    }

    // 2. METADATA (FA)
    while (true) {
        uint8_t opcode = read8(in);
        if (opcode != 0xFA) {
            // rollback 1 byte; this starts DB section
            in.seekg(-1, ios::cur);
            break;
        }

        string metaKey = decodeString(in);
        string metaVal = decodeString(in);
        // You can store metadata if needed
    }

    // 3. Database selector (FE)
    uint8_t dbOP = read8(in);
    if (dbOP != 0xFE)
        throw runtime_error("Missing FE (database selector)");

    uint64_t dbIndex = decodeLength(in, *(new bool(false)));
    // Only DB0 supported

    // 4. Hash table sizes (FB)
    uint8_t fb = read8(in);
    if (fb != 0xFB)
        throw runtime_error("Missing FB hash table size section");

    bool dummy;
    uint64_t keyCount = decodeLength(in, dummy);
    uint64_t expiryCount = decodeLength(in, dummy);

    // redisDb.store.clear();
    // redisDb.expiry.clear();

    // 5. READ KEYS
    while (true) {
        uint8_t op = read8(in);

        if (op == 0xFF) break;  // end-of-file section

        long long expiry = -1;

        // Expiry opcodes
        if (op == 0xFD) {
            // expiry in seconds
            expiry = (long long)read32le(in) * 1000LL;
            op = read8(in);
        }
        else if (op == 0xFC) {
            // expiry in milliseconds
            expiry = (long long)read64le(in);
            op = read8(in);
        }

        // VALUE TYPE (expect 0 = string)
        if (op != 0x00)
            throw runtime_error("Unsupported value type");

        // Decode key
        string key = decodeString(in);
		res.push_back(key);
        // Decode value
        string value = decodeString(in);

        redisDb.store[key] = value;
		if(expiry != -1) redisDb.expiry[key] = expiry;
    }

    // 6. CHECKSUM
    uint64_t crc = read64le(in);
    // TODO: compute CRC64 and compare for validation

    std::cout << "RDB load complete. Keys loaded: " << redisDb.store.size() << endl;
	return encodeToRESPArray(res);
}

