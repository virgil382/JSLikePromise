#pragma once

#include <string>

using namespace std;

/**
 * TranscriptionCounter counts the number of times the transcription operators of this class are called.  Thranscription
 * operators are:
 *   - copy constructor
 *   - move constructor
 *   - copy assignment operator
 *   - move assignment operator
 * Counting is done only for the cohort of an instance (including the instance).  The cohort includes an initial instance
 * and all the copies and copies of copies etc. of the initial instance.  The cohort also includes instances to which
 * cohort members were moved (via move constructor or move assignment operator).
 * 
 * This class is used to verify that the value Promise works as expected in both copy-mode and move-mode.
 */
class TranscriptionCounter {
	std::string objName;
	int* nMoveCtor;
	int* nMoveAssign;
	int* nCopyCtor;
	int* nCopyAssign;

public:
	TranscriptionCounter() = default;

	static TranscriptionCounter* constructAndSetCounters(string const& objName, int* nMoveCtor, int* nMoveAssign, int* nCopyCtor, int* nCopyAssign) {
		auto obj = new TranscriptionCounter();
		obj->objName = objName;

		obj->nMoveCtor = nMoveCtor;
		obj->nMoveAssign = nMoveAssign;
		obj->nCopyCtor = nCopyCtor;
		obj->nCopyAssign = nCopyAssign;
		return obj;
	}

public:


	// Move costructor - pilfer the resources of an rvalue.
	TranscriptionCounter(TranscriptionCounter&& rhs) noexcept {
		this->objName = rhs.objName;  rhs.objName = "";

		this->nMoveCtor = rhs.nMoveCtor;  rhs.nMoveCtor = nullptr;
		this->nMoveAssign = rhs.nMoveAssign;  rhs.nMoveAssign = nullptr;

		this->nCopyCtor = rhs.nCopyCtor;  rhs.nCopyCtor = nullptr;
		this->nCopyAssign = rhs.nCopyAssign;  rhs.nCopyAssign = nullptr;

		(*(this->nMoveCtor))++;
	}

	// Move assign operator - pilfer the resources of an rvalue.
	TranscriptionCounter& operator= (TranscriptionCounter&& rhs) noexcept {
		this->objName = rhs.objName;  rhs.objName = "";

		this->nMoveCtor = rhs.nMoveCtor;  rhs.nMoveCtor = nullptr;
		this->nMoveAssign = rhs.nMoveAssign;  rhs.nMoveAssign = nullptr;

		this->nCopyCtor = rhs.nCopyCtor;  rhs.nCopyCtor = nullptr;
		this->nCopyAssign = rhs.nCopyAssign;  rhs.nCopyAssign = nullptr;

		(*(this->nMoveAssign))++;

		return *this;
	}

	// Copy constructor - copy but do NOT pilfer the resource of rhs
	TranscriptionCounter(const TranscriptionCounter& rhs) {
		this->objName = rhs.objName;

		this->nMoveCtor = rhs.nMoveCtor;
		this->nMoveAssign = rhs.nMoveAssign;

		this->nCopyCtor = rhs.nCopyCtor;
		this->nCopyAssign = rhs.nCopyAssign;

		(*(this->nCopyCtor))++;
	}

	// Copy assign operator - copy but do NOT pilfer the resource of rhs
	TranscriptionCounter &operator= (const TranscriptionCounter& rhs) {
		this->nMoveCtor = rhs.nMoveCtor;
		this->nMoveAssign = rhs.nMoveAssign;

		this->nCopyCtor = rhs.nCopyCtor;
		this->nCopyAssign = rhs.nCopyAssign;

		(*(this->nCopyAssign))++;

		return *this;
	}

	virtual ~TranscriptionCounter() {}

	std::string name() const { return objName; }
};
