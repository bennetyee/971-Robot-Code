#include "aos/events/logging/log_namer.h"

#include <functional>
#include <map>
#include <memory>
#include <string_view>
#include <vector>

#include "absl/strings/str_cat.h"
#include "aos/events/logging/logfile_utils.h"
#include "aos/events/logging/logger_generated.h"
#include "aos/events/logging/uuid.h"
#include "flatbuffers/flatbuffers.h"
#include "glog/logging.h"

namespace aos {
namespace logger {

void LogNamer::UpdateHeader(
    aos::SizePrefixedFlatbufferDetachedBuffer<LogFileHeader> *header,
    const UUID &uuid, int parts_index) const {
  header->mutable_message()->mutate_parts_index(parts_index);
  CHECK_EQ(uuid.string_view().size(),
           header->mutable_message()->mutable_parts_uuid()->size());
  std::copy(uuid.string_view().begin(), uuid.string_view().end(),
            reinterpret_cast<char *>(
                header->mutable_message()->mutable_parts_uuid()->Data()));
}

void LocalLogNamer::WriteHeader(
    aos::SizePrefixedFlatbufferDetachedBuffer<LogFileHeader> *header,
    const Node *node) {
  CHECK_EQ(node, this->node());
  UpdateHeader(header, uuid_, part_number_);
  data_writer_->WriteSizedFlatbuffer(header->full_span());
}

DetachedBufferWriter *LocalLogNamer::MakeWriter(const Channel *channel) {
  CHECK(configuration::ChannelIsSendableOnNode(channel, node()));
  return data_writer_.get();
}

void LocalLogNamer::Rotate(
    const Node *node,
    aos::SizePrefixedFlatbufferDetachedBuffer<LogFileHeader> *header) {
  CHECK(node == this->node());
  ++part_number_;
  *data_writer_ = std::move(*OpenDataWriter());
  UpdateHeader(header, uuid_, part_number_);
  data_writer_->WriteSizedFlatbuffer(header->full_span());
}

DetachedBufferWriter *LocalLogNamer::MakeTimestampWriter(
    const Channel *channel) {
  CHECK(configuration::ChannelIsReadableOnNode(channel, node_))
      << ": Message is not delivered to this node.";
  CHECK(node_ != nullptr) << ": Can't log timestamps in a single node world";
  CHECK(configuration::ConnectionDeliveryTimeIsLoggedOnNode(channel, node_,
                                                            node_))
      << ": Delivery times aren't logged for this channel on this node.";
  return data_writer_.get();
}

DetachedBufferWriter *LocalLogNamer::MakeForwardedTimestampWriter(
    const Channel * /*channel*/, const Node * /*node*/) {
  LOG(FATAL) << "Can't log forwarded timestamps in a singe log file.";
  return nullptr;
}

MultiNodeLogNamer::MultiNodeLogNamer(std::string_view base_name,
                                     const Configuration *configuration,
                                     const Node *node)
    : LogNamer(node),
      base_name_(base_name),
      configuration_(configuration),
      uuid_(UUID::Random()),
      data_writer_(OpenDataWriter()) {}

void MultiNodeLogNamer::WriteHeader(
    aos::SizePrefixedFlatbufferDetachedBuffer<LogFileHeader> *header,
    const Node *node) {
  if (node == this->node()) {
    UpdateHeader(header, uuid_, part_number_);
    data_writer_->WriteSizedFlatbuffer(header->full_span());
  } else {
    for (std::pair<const Channel *const, DataWriter> &data_writer :
         data_writers_) {
      if (node == data_writer.second.node) {
        UpdateHeader(header, data_writer.second.uuid,
                     data_writer.second.part_number);
        data_writer.second.writer->WriteSizedFlatbuffer(header->full_span());
      }
    }
  }
}

void MultiNodeLogNamer::Rotate(
    const Node *node,
    aos::SizePrefixedFlatbufferDetachedBuffer<LogFileHeader> *header) {
  if (node == this->node()) {
    ++part_number_;
    *data_writer_ = std::move(*OpenDataWriter());
    UpdateHeader(header, uuid_, part_number_);
    data_writer_->WriteSizedFlatbuffer(header->full_span());
  } else {
    for (std::pair<const Channel *const, DataWriter> &data_writer :
         data_writers_) {
      if (node == data_writer.second.node) {
        ++data_writer.second.part_number;
        data_writer.second.rotate(data_writer.first, &data_writer.second);
        UpdateHeader(header, data_writer.second.uuid,
                     data_writer.second.part_number);
        data_writer.second.writer->WriteSizedFlatbuffer(header->full_span());
      }
    }
  }
}

DetachedBufferWriter *MultiNodeLogNamer::MakeWriter(const Channel *channel) {
  // See if we can read the data on this node at all.
  const bool is_readable =
      configuration::ChannelIsReadableOnNode(channel, this->node());
  if (!is_readable) {
    return nullptr;
  }

  // Then, see if we are supposed to log the data here.
  const bool log_message =
      configuration::ChannelMessageIsLoggedOnNode(channel, this->node());

  if (!log_message) {
    return nullptr;
  }

  // Now, sort out if this is data generated on this node, or not.  It is
  // generated if it is sendable on this node.
  if (configuration::ChannelIsSendableOnNode(channel, this->node())) {
    return data_writer_.get();
  }

  // Ok, we have data that is being forwarded to us that we are supposed to
  // log.  It needs to be logged with send timestamps, but be sorted enough
  // to be able to be processed.
  CHECK(data_writers_.find(channel) == data_writers_.end());

  // Track that this node is being logged.
  const Node *source_node = configuration::GetNode(
      configuration_, channel->source_node()->string_view());

  if (std::find(nodes_.begin(), nodes_.end(), source_node) == nodes_.end()) {
    nodes_.emplace_back(source_node);
  }

  DataWriter data_writer;
  data_writer.node = source_node;
  data_writer.rotate = [this](const Channel *channel, DataWriter *data_writer) {
    OpenWriter(channel, data_writer);
  };
  data_writer.rotate(channel, &data_writer);

  return data_writers_.insert(std::make_pair(channel, std::move(data_writer)))
      .first->second.writer.get();
}

DetachedBufferWriter *MultiNodeLogNamer::MakeForwardedTimestampWriter(
    const Channel *channel, const Node *node) {
  // See if we can read the data on this node at all.
  const bool is_readable =
      configuration::ChannelIsReadableOnNode(channel, this->node());
  CHECK(is_readable) << ": " << configuration::CleanedChannelToString(channel);

  CHECK(data_writers_.find(channel) == data_writers_.end());

  if (std::find(nodes_.begin(), nodes_.end(), node) == nodes_.end()) {
    nodes_.emplace_back(node);
  }

  DataWriter data_writer;
  data_writer.node = node;
  data_writer.rotate = [this](const Channel *channel, DataWriter *data_writer) {
    OpenForwardedTimestampWriter(channel, data_writer);
  };
  data_writer.rotate(channel, &data_writer);

  return data_writers_.insert(std::make_pair(channel, std::move(data_writer)))
      .first->second.writer.get();
}

DetachedBufferWriter *MultiNodeLogNamer::MakeTimestampWriter(
    const Channel *channel) {
  const bool log_delivery_times =
      (this->node() == nullptr)
          ? false
          : configuration::ConnectionDeliveryTimeIsLoggedOnNode(
                channel, this->node(), this->node());
  if (!log_delivery_times) {
    return nullptr;
  }

  return data_writer_.get();
}

void MultiNodeLogNamer::OpenForwardedTimestampWriter(const Channel *channel,
                                                     DataWriter *data_writer) {
  std::string filename =
      absl::StrCat(base_name_, "_timestamps", channel->name()->string_view(),
                   "/", channel->type()->string_view(), ".part",
                   data_writer->part_number, ".bfbs");

  if (!data_writer->writer) {
    data_writer->writer = std::make_unique<DetachedBufferWriter>(filename);
  } else {
    *data_writer->writer = DetachedBufferWriter(filename);
  }
}

void MultiNodeLogNamer::OpenWriter(const Channel *channel,
                                   DataWriter *data_writer) {
  const std::string filename = absl::StrCat(
      base_name_, "_", channel->source_node()->string_view(), "_data",
      channel->name()->string_view(), "/", channel->type()->string_view(),
      ".part", data_writer->part_number, ".bfbs");
  if (!data_writer->writer) {
    data_writer->writer = std::make_unique<DetachedBufferWriter>(filename);
  } else {
    *data_writer->writer = DetachedBufferWriter(filename);
  }
}

std::unique_ptr<DetachedBufferWriter> MultiNodeLogNamer::OpenDataWriter() {
  return std::make_unique<DetachedBufferWriter>(
      absl::StrCat(base_name_, "_", node()->name()->string_view(), "_data.part",
                   part_number_, ".bfbs"));
}

}  // namespace logger
}  // namespace aos