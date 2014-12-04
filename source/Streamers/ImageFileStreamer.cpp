#include "ImageFileImporter.hpp"
#include "DeviceManager.hpp"
#include "Exception.hpp"
#include <boost/lexical_cast.hpp>
#include "ImageFileStreamer.hpp"

namespace fast {
/**
 * Dummy function to get into the class again
 */
inline void stubStreamThread(ImageFileStreamer * streamer) {
    streamer->producerStream();
}

DynamicImage::pointer ImageFileStreamer::getOutput() {
    mOutput->setSource(mPtr.lock());
    mOutput->setStreamer(mPtr.lock());
    return mOutput;
}

ImageFileStreamer::ImageFileStreamer() {
    mOutput = DynamicImage::New();
    mStreamIsStarted = false;
    mIsModified = true;
    mLoop = false;
    mStartNumber = 0;
    mZeroFillDigits = 0;
    thread = NULL;
    mFirstFrameIsInserted = false;
    mHasReachedEnd = false;
    mFilenameFormat = "";
    mNrOfFrames = 0;
    mDevice = DeviceManager::getInstance().getDefaultComputationDevice();
}

uint ImageFileStreamer::getNrOfFrames() const {
    return mNrOfFrames;
}

void ImageFileStreamer::execute() {
    mOutput->setSource(mPtr.lock());
    mOutput->setStreamer(mPtr.lock());
    if(mFilenameFormat == "")
        throw Exception("No filename format was given to the ImageFileStreamer");
    if(!mStreamIsStarted) {
        // Check that first frame exists before starting streamer

        mStreamIsStarted = true;
        thread = new boost::thread(&stubStreamThread, this);
    }

    // Wait here for first frame
    boost::unique_lock<boost::mutex> lock(mFirstFrameMutex);
    while(!mFirstFrameIsInserted) {
        mFirstFrameCondition.wait(lock);
    }
}

void ImageFileStreamer::setFilenameFormat(std::string str) {
    if(str.find("#") == std::string::npos)
        throw Exception("Filename format must include a hash tag # which will be replaced by a integer starting from 0.");
    mFilenameFormat = str;
}

void ImageFileStreamer::setDevice(ExecutionDevice::pointer device) {
    mDevice = device;
}


void ImageFileStreamer::producerStream() {
    uint i = mStartNumber;
    while(true) {
        std::string filename = mFilenameFormat;
        std::string frameNumber = boost::lexical_cast<std::string>(i);
        if(mZeroFillDigits > 0 && frameNumber.size() < mZeroFillDigits) {
            std::string zeroFilling = "";
            for(uint z = 0; z < mZeroFillDigits-frameNumber.size(); z++) {
                zeroFilling += "0";
            }
            frameNumber = zeroFilling + frameNumber;
        }
        filename.replace(
                filename.find("#"),
                1,
                frameNumber
                );
        try {
            ImageFileImporter::pointer importer = ImageFileImporter::New();
            importer->setFilename(filename);
            importer->setMainDevice(mDevice);
            Image::pointer image = importer->getOutput();
            image->update();
            DynamicImage::pointer ptr = mOutput;
            if(ptr.isValid()) {
                try {
                    ptr->addFrame(image);
                } catch(Exception &e) {
                    std::cout << "streamer has been deleted, stop" << std::endl;
                    break;
                }
                {
                    boost::lock_guard<boost::mutex> lock(mFirstFrameMutex);
                    mFirstFrameIsInserted = true;
                }
                mFirstFrameCondition.notify_one();
            } else {
                std::cout << "DynamicImage object destroyed, stream can stop." << std::endl;
                break;
            }
            mNrOfFrames++;
            i++;
        } catch(FileNotFoundException &e) {
            if(i > 0) {
                std::cout << "Reached end of stream" << std::endl;
                // If there where no files found at all, we need to release the execute method
                {
                    boost::lock_guard<boost::mutex> lock(mFirstFrameMutex);
                    mFirstFrameIsInserted = true;
                }
                mFirstFrameCondition.notify_one();
                if(mLoop) {
                    // Restart stream
                    i = mStartNumber;
                    continue;
                }
                mHasReachedEnd = true;
                // Reached end of stream
                break;
            } else {
                throw e;
            }
        }
    }
}

ImageFileStreamer::~ImageFileStreamer() {
    if(mStreamIsStarted) {
        if(thread->get_id() != boost::this_thread::get_id()) { // avoid deadlock
            thread->join();
        }
        delete thread;
    }
}

bool ImageFileStreamer::hasReachedEnd() const {
    return mHasReachedEnd;
}

void ImageFileStreamer::setStartNumber(uint startNumber) {
    mStartNumber = startNumber;
}

void ImageFileStreamer::setZeroFilling(uint digits) {
    mZeroFillDigits = digits;
}

void ImageFileStreamer::enableLooping() {
    mLoop = true;
}

void ImageFileStreamer::disableLooping() {
    mLoop = false;
}

} // end namespace fast