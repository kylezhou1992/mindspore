/**
 * Copyright 2019 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <exception>

#include "minddata/dataset/api/de_pipeline.h"
#include "minddata/dataset/engine/datasetops/source/cifar_op.h"
#include "minddata/dataset/engine/datasetops/source/clue_op.h"
#include "minddata/dataset/engine/datasetops/source/coco_op.h"
#include "minddata/dataset/engine/datasetops/source/image_folder_op.h"
#include "minddata/dataset/engine/datasetops/source/io_block.h"
#include "minddata/dataset/engine/datasetops/source/manifest_op.h"
#include "minddata/dataset/engine/datasetops/source/mindrecord_op.h"
#include "minddata/dataset/engine/datasetops/source/mnist_op.h"
#include "minddata/dataset/engine/datasetops/source/random_data_op.h"
#include "minddata/dataset/engine/datasetops/source/sampler/distributed_sampler.h"
#include "minddata/dataset/engine/datasetops/source/sampler/pk_sampler.h"
#include "minddata/dataset/engine/datasetops/source/sampler/python_sampler.h"
#include "minddata/dataset/engine/datasetops/source/sampler/random_sampler.h"
#include "minddata/dataset/engine/datasetops/source/sampler/sequential_sampler.h"
#include "minddata/dataset/engine/datasetops/source/sampler/subset_random_sampler.h"
#include "minddata/dataset/engine/datasetops/source/sampler/weighted_random_sampler.h"
#include "minddata/dataset/engine/datasetops/source/text_file_op.h"
#include "minddata/dataset/engine/datasetops/source/tf_reader_op.h"
#include "minddata/dataset/engine/datasetops/source/voc_op.h"
#include "minddata/dataset/engine/cache/cache_client.h"
#include "minddata/dataset/engine/gnn/graph.h"
#include "minddata/dataset/engine/jagged_connector.h"
#include "minddata/dataset/kernels/data/concatenate_op.h"
#include "minddata/dataset/kernels/data/duplicate_op.h"
#include "minddata/dataset/kernels/data/fill_op.h"
#include "minddata/dataset/kernels/data/mask_op.h"
#include "minddata/dataset/kernels/data/one_hot_op.h"
#include "minddata/dataset/kernels/data/pad_end_op.h"
#include "minddata/dataset/kernels/data/slice_op.h"
#include "minddata/dataset/kernels/data/to_float16_op.h"
#include "minddata/dataset/kernels/data/type_cast_op.h"
#include "minddata/dataset/kernels/image/bounding_box_augment_op.h"
#include "minddata/dataset/kernels/image/center_crop_op.h"
#include "minddata/dataset/kernels/image/cut_out_op.h"
#include "minddata/dataset/kernels/image/decode_op.h"
#include "minddata/dataset/kernels/image/hwc_to_chw_op.h"
#include "minddata/dataset/kernels/image/image_utils.h"
#include "minddata/dataset/kernels/image/normalize_op.h"
#include "minddata/dataset/kernels/image/pad_op.h"
#include "minddata/dataset/kernels/image/random_color_adjust_op.h"
#include "minddata/dataset/kernels/image/random_crop_and_resize_op.h"
#include "minddata/dataset/kernels/image/random_crop_and_resize_with_bbox_op.h"
#include "minddata/dataset/kernels/image/random_crop_decode_resize_op.h"
#include "minddata/dataset/kernels/image/random_crop_op.h"
#include "minddata/dataset/kernels/image/random_crop_with_bbox_op.h"
#include "minddata/dataset/kernels/image/random_horizontal_flip_with_bbox_op.h"
#include "minddata/dataset/kernels/image/random_horizontal_flip_op.h"
#include "minddata/dataset/kernels/image/random_resize_op.h"
#include "minddata/dataset/kernels/image/random_resize_with_bbox_op.h"
#include "minddata/dataset/kernels/image/random_rotation_op.h"
#include "minddata/dataset/kernels/image/random_vertical_flip_op.h"
#include "minddata/dataset/kernels/image/random_vertical_flip_with_bbox_op.h"
#include "minddata/dataset/kernels/image/rescale_op.h"
#include "minddata/dataset/kernels/image/resize_bilinear_op.h"
#include "minddata/dataset/kernels/image/resize_op.h"
#include "minddata/dataset/kernels/image/resize_with_bbox_op.h"
#include "minddata/dataset/kernels/image/uniform_aug_op.h"
#include "minddata/dataset/kernels/no_op.h"
#include "minddata/dataset/text/kernels/jieba_tokenizer_op.h"
#include "minddata/dataset/text/kernels/lookup_op.h"
#include "minddata/dataset/text/kernels/ngram_op.h"
#include "minddata/dataset/text/kernels/to_number_op.h"
#include "minddata/dataset/text/kernels/unicode_char_tokenizer_op.h"
#include "minddata/dataset/text/kernels/wordpiece_tokenizer_op.h"
#include "minddata/dataset/text/vocab.h"
#include "minddata/dataset/util/random.h"
#include "minddata/mindrecord/include/shard_distributed_sample.h"
#include "minddata/mindrecord/include/shard_operator.h"
#include "minddata/mindrecord/include/shard_pk_sample.h"
#include "minddata/mindrecord/include/shard_sample.h"
#include "minddata/mindrecord/include/shard_sequential_sample.h"
#include "mindspore/ccsrc/minddata/dataset/text/kernels/truncate_sequence_pair_op.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/stl_bind.h"

#ifdef ENABLE_ICU4C
#include "minddata/dataset/text/kernels/basic_tokenizer_op.h"
#include "minddata/dataset/text/kernels/bert_tokenizer_op.h"
#include "minddata/dataset/text/kernels/case_fold_op.h"
#include "minddata/dataset/text/kernels/normalize_utf8_op.h"
#include "minddata/dataset/text/kernels/regex_replace_op.h"
#include "minddata/dataset/text/kernels/regex_tokenizer_op.h"
#include "minddata/dataset/text/kernels/unicode_script_tokenizer_op.h"
#include "minddata/dataset/text/kernels/whitespace_tokenizer_op.h"
#endif

namespace py = pybind11;

namespace mindspore {
namespace dataset {
#define THROW_IF_ERROR(s)                                      \
  do {                                                         \
    Status rc = std::move(s);                                  \
    if (rc.IsError()) throw std::runtime_error(rc.ToString()); \
  } while (false)

void bindDEPipeline(py::module *m) {
  (void)py::class_<DEPipeline>(*m, "DEPipeline")
    .def(py::init<>())
    .def(
      "AddNodeToTree",
      [](DEPipeline &de, const OpName &op_name, const py::dict &args) {
        py::dict out;
        THROW_IF_ERROR(de.AddNodeToTree(op_name, args, &out));
        return out;
      },
      py::return_value_policy::reference)
    .def_static("AddChildToParentNode",
                [](const DsOpPtr &child_op, const DsOpPtr &parent_op) {
                  THROW_IF_ERROR(DEPipeline::AddChildToParentNode(child_op, parent_op));
                })
    .def("AssignRootNode",
         [](DEPipeline &de, const DsOpPtr &dataset_op) { THROW_IF_ERROR(de.AssignRootNode(dataset_op)); })
    .def("SetBatchParameters",
         [](DEPipeline &de, const py::dict &args) { THROW_IF_ERROR(de.SetBatchParameters(args)); })
    .def("LaunchTreeExec", [](DEPipeline &de) { THROW_IF_ERROR(de.LaunchTreeExec()); })
    .def("GetNextAsMap",
         [](DEPipeline &de) {
           py::dict out;
           THROW_IF_ERROR(de.GetNextAsMap(&out));
           return out;
         })
    .def("GetNextAsList",
         [](DEPipeline &de) {
           py::list out;
           THROW_IF_ERROR(de.GetNextAsList(&out));
           return out;
         })
    .def("GetOutputShapes",
         [](DEPipeline &de) {
           py::list out;
           THROW_IF_ERROR(de.GetOutputShapes(&out));
           return out;
         })
    .def("GetOutputTypes",
         [](DEPipeline &de) {
           py::list out;
           THROW_IF_ERROR(de.GetOutputTypes(&out));
           return out;
         })
    .def("GetDatasetSize", &DEPipeline::GetDatasetSize)
    .def("GetBatchSize", &DEPipeline::GetBatchSize)
    .def("GetNumClasses", &DEPipeline::GetNumClasses)
    .def("GetRepeatCount", &DEPipeline::GetRepeatCount);
}
void bindDatasetOps(py::module *m) {
  (void)py::class_<TFReaderOp, DatasetOp, std::shared_ptr<TFReaderOp>>(*m, "TFReaderOp")
    .def_static("get_num_rows", [](const py::list &files, int64_t numParallelWorkers, bool estimate = false) {
      int64_t count = 0;
      std::vector<std::string> filenames;
      for (auto l : files) {
        !l.is_none() ? filenames.push_back(py::str(l)) : (void)filenames.emplace_back("");
      }
      THROW_IF_ERROR(TFReaderOp::CountTotalRows(&count, filenames, numParallelWorkers, estimate));
      return count;
    });

  (void)py::class_<CifarOp, DatasetOp, std::shared_ptr<CifarOp>>(*m, "CifarOp")
    .def_static("get_num_rows", [](const std::string &dir, bool isCifar10) {
      int64_t count = 0;
      THROW_IF_ERROR(CifarOp::CountTotalRows(dir, isCifar10, &count));
      return count;
    });

  (void)py::class_<ImageFolderOp, DatasetOp, std::shared_ptr<ImageFolderOp>>(*m, "ImageFolderOp")
    .def_static("get_num_rows_and_classes", [](const std::string &path) {
      int64_t count = 0, num_classes = 0;
      THROW_IF_ERROR(ImageFolderOp::CountRowsAndClasses(path, std::set<std::string>{}, &count, &num_classes));
      return py::make_tuple(count, num_classes);
    });

  (void)py::class_<MindRecordOp, DatasetOp, std::shared_ptr<MindRecordOp>>(*m, "MindRecordOp")
    .def_static("get_num_rows", [](const std::vector<std::string> &paths, bool load_dataset, const py::object &sampler,
                                   const int64_t num_padded) {
      int64_t count = 0;
      std::shared_ptr<mindrecord::ShardOperator> op;
      if (py::hasattr(sampler, "create_for_minddataset")) {
        auto create = sampler.attr("create_for_minddataset");
        op = create().cast<std::shared_ptr<mindrecord::ShardOperator>>();
      }
      THROW_IF_ERROR(MindRecordOp::CountTotalRows(paths, load_dataset, op, &count, num_padded));
      return count;
    });

  (void)py::class_<ManifestOp, DatasetOp, std::shared_ptr<ManifestOp>>(*m, "ManifestOp")
    .def_static("get_num_rows_and_classes",
                [](const std::string &file, const py::dict &dict, const std::string &usage) {
                  int64_t count = 0, num_classes = 0;
                  THROW_IF_ERROR(ManifestOp::CountTotalRows(file, dict, usage, &count, &num_classes));
                  return py::make_tuple(count, num_classes);
                })
    .def_static("get_class_indexing", [](const std::string &file, const py::dict &dict, const std::string &usage) {
      std::map<std::string, int32_t> output_class_indexing;
      THROW_IF_ERROR(ManifestOp::GetClassIndexing(file, dict, usage, &output_class_indexing));
      return output_class_indexing;
    });

  (void)py::class_<MnistOp, DatasetOp, std::shared_ptr<MnistOp>>(*m, "MnistOp")
    .def_static("get_num_rows", [](const std::string &dir) {
      int64_t count = 0;
      THROW_IF_ERROR(MnistOp::CountTotalRows(dir, &count));
      return count;
    });

  (void)py::class_<TextFileOp, DatasetOp, std::shared_ptr<TextFileOp>>(*m, "TextFileOp")
    .def_static("get_num_rows", [](const py::list &files) {
      int64_t count = 0;
      std::vector<std::string> filenames;
      for (auto file : files) {
        !file.is_none() ? filenames.push_back(py::str(file)) : (void)filenames.emplace_back("");
      }
      THROW_IF_ERROR(TextFileOp::CountAllFileRows(filenames, &count));
      return count;
    });

  (void)py::class_<ClueOp, DatasetOp, std::shared_ptr<ClueOp>>(*m, "ClueOp")
    .def_static("get_num_rows", [](const py::list &files) {
      int64_t count = 0;
      std::vector<std::string> filenames;
      for (auto file : files) {
        file.is_none() ? (void)filenames.emplace_back("") : filenames.push_back(py::str(file));
      }
      THROW_IF_ERROR(ClueOp::CountAllFileRows(filenames, &count));
      return count;
    });

  (void)py::class_<VOCOp, DatasetOp, std::shared_ptr<VOCOp>>(*m, "VOCOp")
    .def_static("get_num_rows",
                [](const std::string &dir, const std::string &task_type, const std::string &task_mode,
                   const py::dict &dict, int64_t numSamples) {
                  int64_t count = 0;
                  THROW_IF_ERROR(VOCOp::CountTotalRows(dir, task_type, task_mode, dict, &count));
                  return count;
                })
    .def_static("get_class_indexing", [](const std::string &dir, const std::string &task_type,
                                         const std::string &task_mode, const py::dict &dict) {
      std::map<std::string, int32_t> output_class_indexing;
      THROW_IF_ERROR(VOCOp::GetClassIndexing(dir, task_type, task_mode, dict, &output_class_indexing));
      return output_class_indexing;
    });
  (void)py::class_<CocoOp, DatasetOp, std::shared_ptr<CocoOp>>(*m, "CocoOp")
    .def_static("get_class_indexing",
                [](const std::string &dir, const std::string &file, const std::string &task) {
                  std::vector<std::pair<std::string, std::vector<int32_t>>> output_class_indexing;
                  THROW_IF_ERROR(CocoOp::GetClassIndexing(dir, file, task, &output_class_indexing));
                  return output_class_indexing;
                })
    .def_static("get_num_rows", [](const std::string &dir, const std::string &file, const std::string &task) {
      int64_t count = 0;
      THROW_IF_ERROR(CocoOp::CountTotalRows(dir, file, task, &count));
      return count;
    });
}
void bindTensor(py::module *m) {
  (void)py::class_<GlobalContext>(*m, "GlobalContext")
    .def_static("config_manager", &GlobalContext::config_manager, py::return_value_policy::reference);

  (void)py::class_<ConfigManager, std::shared_ptr<ConfigManager>>(*m, "ConfigManager")
    .def("__str__", &ConfigManager::ToString)
    .def("set_rows_per_buffer", &ConfigManager::set_rows_per_buffer)
    .def("set_num_parallel_workers", &ConfigManager::set_num_parallel_workers)
    .def("set_worker_connector_size", &ConfigManager::set_worker_connector_size)
    .def("set_op_connector_size", &ConfigManager::set_op_connector_size)
    .def("set_seed", &ConfigManager::set_seed)
    .def("set_monitor_sampling_interval", &ConfigManager::set_monitor_sampling_interval)
    .def("get_rows_per_buffer", &ConfigManager::rows_per_buffer)
    .def("get_num_parallel_workers", &ConfigManager::num_parallel_workers)
    .def("get_worker_connector_size", &ConfigManager::worker_connector_size)
    .def("get_op_connector_size", &ConfigManager::op_connector_size)
    .def("get_seed", &ConfigManager::seed)
    .def("get_monitor_sampling_interval", &ConfigManager::monitor_sampling_interval)
    .def("load", [](ConfigManager &c, std::string s) { THROW_IF_ERROR(c.LoadFile(s)); });

  (void)py::class_<Tensor, std::shared_ptr<Tensor>>(*m, "Tensor", py::buffer_protocol())
    .def(py::init([](py::array arr) {
      std::shared_ptr<Tensor> out;
      THROW_IF_ERROR(Tensor::CreateTensor(&out, arr));
      return out;
    }))
    .def_buffer([](Tensor &tensor) {
      py::buffer_info info;
      THROW_IF_ERROR(Tensor::GetBufferInfo(&tensor, &info));
      return info;
    })
    .def("__str__", &Tensor::ToString)
    .def("shape", &Tensor::shape)
    .def("type", &Tensor::type)
    .def("as_array", [](py::object &t) {
      auto &tensor = py::cast<Tensor &>(t);
      if (tensor.type() == DataType::DE_STRING) {
        py::array res;
        tensor.GetDataAsNumpyStrings(&res);
        return res;
      }
      py::buffer_info info;
      THROW_IF_ERROR(Tensor::GetBufferInfo(&tensor, &info));
      return py::array(pybind11::dtype(info), info.shape, info.strides, info.ptr, t);
    });

  (void)py::class_<TensorShape>(*m, "TensorShape")
    .def(py::init<py::list>())
    .def("__str__", &TensorShape::ToString)
    .def("as_list", &TensorShape::AsPyList)
    .def("is_known", &TensorShape::known);

  (void)py::class_<DataType>(*m, "DataType")
    .def(py::init<std::string>())
    .def(py::self == py::self)
    .def("__str__", &DataType::ToString)
    .def("__deepcopy__", [](py::object &t, py::dict memo) { return t; });
}

void bindTensorOps1(py::module *m) {
  (void)py::class_<TensorOp, std::shared_ptr<TensorOp>>(*m, "TensorOp")
    .def("__deepcopy__", [](py::object &t, py::dict memo) { return t; });

  (void)py::class_<NormalizeOp, TensorOp, std::shared_ptr<NormalizeOp>>(
    *m, "NormalizeOp", "Tensor operation to normalize an image. Takes mean and std.")
    .def(py::init<float, float, float, float, float, float>(), py::arg("meanR"), py::arg("meanG"), py::arg("meanB"),
         py::arg("stdR"), py::arg("stdG"), py::arg("stdB"));

  (void)py::class_<RescaleOp, TensorOp, std::shared_ptr<RescaleOp>>(
    *m, "RescaleOp", "Tensor operation to rescale an image. Takes scale and shift.")
    .def(py::init<float, float>(), py::arg("rescale"), py::arg("shift"));

  (void)py::class_<CenterCropOp, TensorOp, std::shared_ptr<CenterCropOp>>(
    *m, "CenterCropOp", "Tensor operation to crop and image in the middle. Takes height and width (optional)")
    .def(py::init<int32_t, int32_t>(), py::arg("height"), py::arg("width") = CenterCropOp::kDefWidth);

  (void)py::class_<ResizeOp, TensorOp, std::shared_ptr<ResizeOp>>(
    *m, "ResizeOp", "Tensor operation to resize an image. Takes height, width and mode")
    .def(py::init<int32_t, int32_t, InterpolationMode>(), py::arg("targetHeight"),
         py::arg("targetWidth") = ResizeOp::kDefWidth, py::arg("interpolation") = ResizeOp::kDefInterpolation);

  (void)py::class_<ResizeWithBBoxOp, TensorOp, std::shared_ptr<ResizeWithBBoxOp>>(
    *m, "ResizeWithBBoxOp", "Tensor operation to resize an image. Takes height, width and mode.")
    .def(py::init<int32_t, int32_t, InterpolationMode>(), py::arg("targetHeight"),
         py::arg("targetWidth") = ResizeWithBBoxOp::kDefWidth,
         py::arg("interpolation") = ResizeWithBBoxOp::kDefInterpolation);

  (void)py::class_<RandomResizeWithBBoxOp, TensorOp, std::shared_ptr<RandomResizeWithBBoxOp>>(
    *m, "RandomResizeWithBBoxOp",
    "Tensor operation to resize an image using a randomly selected interpolation. Takes height and width.")
    .def(py::init<int32_t, int32_t>(), py::arg("targetHeight"),
         py::arg("targetWidth") = RandomResizeWithBBoxOp::kDefTargetWidth);

  (void)py::class_<UniformAugOp, TensorOp, std::shared_ptr<UniformAugOp>>(
    *m, "UniformAugOp", "Tensor operation to apply random augmentation(s).")
    .def(py::init<std::vector<std::shared_ptr<TensorOp>>, int32_t>(), py::arg("operations"),
         py::arg("NumOps") = UniformAugOp::kDefNumOps);

  (void)py::class_<BoundingBoxAugmentOp, TensorOp, std::shared_ptr<BoundingBoxAugmentOp>>(
    *m, "BoundingBoxAugmentOp", "Tensor operation to apply a transformation on a random choice of bounding boxes.")
    .def(py::init<std::shared_ptr<TensorOp>, float>(), py::arg("transform"),
         py::arg("ratio") = BoundingBoxAugmentOp::kDefRatio);

  (void)py::class_<ResizeBilinearOp, TensorOp, std::shared_ptr<ResizeBilinearOp>>(
    *m, "ResizeBilinearOp",
    "Tensor operation to resize an image using "
    "Bilinear mode. Takes height and width.")
    .def(py::init<int32_t, int32_t>(), py::arg("targetHeight"), py::arg("targetWidth") = ResizeBilinearOp::kDefWidth);

  (void)py::class_<DecodeOp, TensorOp, std::shared_ptr<DecodeOp>>(*m, "DecodeOp",
                                                                  "Tensor operation to decode a jpg image")
    .def(py::init<>())
    .def(py::init<bool>(), py::arg("rgb_format") = DecodeOp::kDefRgbFormat);

  (void)py::class_<RandomHorizontalFlipOp, TensorOp, std::shared_ptr<RandomHorizontalFlipOp>>(
    *m, "RandomHorizontalFlipOp", "Tensor operation to randomly flip an image horizontally.")
    .def(py::init<float>(), py::arg("probability") = RandomHorizontalFlipOp::kDefProbability);

  (void)py::class_<RandomHorizontalFlipWithBBoxOp, TensorOp, std::shared_ptr<RandomHorizontalFlipWithBBoxOp>>(
    *m, "RandomHorizontalFlipWithBBoxOp",
    "Tensor operation to randomly flip an image horizontally, while flipping bounding boxes.")
    .def(py::init<float>(), py::arg("probability") = RandomHorizontalFlipWithBBoxOp::kDefProbability);
}

void bindTensorOps2(py::module *m) {
  (void)py::class_<RandomVerticalFlipOp, TensorOp, std::shared_ptr<RandomVerticalFlipOp>>(
    *m, "RandomVerticalFlipOp", "Tensor operation to randomly flip an image vertically.")
    .def(py::init<float>(), py::arg("probability") = RandomVerticalFlipOp::kDefProbability);

  (void)py::class_<RandomVerticalFlipWithBBoxOp, TensorOp, std::shared_ptr<RandomVerticalFlipWithBBoxOp>>(
    *m, "RandomVerticalFlipWithBBoxOp",
    "Tensor operation to randomly flip an image vertically"
    " and adjust bounding boxes.")
    .def(py::init<float>(), py::arg("probability") = RandomVerticalFlipWithBBoxOp::kDefProbability);

  (void)py::class_<RandomCropOp, TensorOp, std::shared_ptr<RandomCropOp>>(*m, "RandomCropOp",
                                                                          "Gives random crop of specified size "
                                                                          "Takes crop size")
    .def(py::init<int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, BorderType, bool, uint8_t, uint8_t, uint8_t>(),
         py::arg("cropHeight"), py::arg("cropWidth"), py::arg("padTop") = RandomCropOp::kDefPadTop,
         py::arg("padBottom") = RandomCropOp::kDefPadBottom, py::arg("padLeft") = RandomCropOp::kDefPadLeft,
         py::arg("padRight") = RandomCropOp::kDefPadRight, py::arg("borderType") = RandomCropOp::kDefBorderType,
         py::arg("padIfNeeded") = RandomCropOp::kDefPadIfNeeded, py::arg("fillR") = RandomCropOp::kDefFillR,
         py::arg("fillG") = RandomCropOp::kDefFillG, py::arg("fillB") = RandomCropOp::kDefFillB);
  (void)py::class_<HwcToChwOp, TensorOp, std::shared_ptr<HwcToChwOp>>(*m, "ChannelSwapOp").def(py::init<>());

  (void)py::class_<RandomCropWithBBoxOp, TensorOp, std::shared_ptr<RandomCropWithBBoxOp>>(*m, "RandomCropWithBBoxOp",
                                                                                          "Gives random crop of given "
                                                                                          "size + adjusts bboxes "
                                                                                          "Takes crop size")
    .def(py::init<int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, BorderType, bool, uint8_t, uint8_t, uint8_t>(),
         py::arg("cropHeight"), py::arg("cropWidth"), py::arg("padTop") = RandomCropWithBBoxOp::kDefPadTop,
         py::arg("padBottom") = RandomCropWithBBoxOp::kDefPadBottom,
         py::arg("padLeft") = RandomCropWithBBoxOp::kDefPadLeft,
         py::arg("padRight") = RandomCropWithBBoxOp::kDefPadRight,
         py::arg("borderType") = RandomCropWithBBoxOp::kDefBorderType,
         py::arg("padIfNeeded") = RandomCropWithBBoxOp::kDefPadIfNeeded,
         py::arg("fillR") = RandomCropWithBBoxOp::kDefFillR, py::arg("fillG") = RandomCropWithBBoxOp::kDefFillG,
         py::arg("fillB") = RandomCropWithBBoxOp::kDefFillB);

  (void)py::class_<OneHotOp, TensorOp, std::shared_ptr<OneHotOp>>(
    *m, "OneHotOp", "Tensor operation to apply one hot encoding. Takes number of classes.")
    .def(py::init<int32_t>());

  (void)py::class_<FillOp, TensorOp, std::shared_ptr<FillOp>>(
    *m, "FillOp", "Tensor operation to return tensor filled with same value as input fill value.")
    .def(py::init<std::shared_ptr<Tensor>>());

  (void)py::class_<SliceOp, TensorOp, std::shared_ptr<SliceOp>>(*m, "SliceOp", "Tensor slice operation.")
    .def(py::init<bool>())
    .def(py::init([](const py::list &py_list) {
      std::vector<dsize_t> c_list;
      for (auto l : py_list) {
        if (!l.is_none()) {
          c_list.push_back(py::reinterpret_borrow<py::int_>(l));
        }
      }
      return std::make_shared<SliceOp>(c_list);
    }))
    .def(py::init([](const py::tuple &py_slice) {
      if (py_slice.size() != 3) {
        THROW_IF_ERROR(Status(StatusCode::kUnexpectedError, __LINE__, __FILE__, "Wrong slice object"));
      }
      Slice c_slice;
      if (!py_slice[0].is_none() && !py_slice[1].is_none() && !py_slice[2].is_none()) {
        c_slice = Slice(py::reinterpret_borrow<py::int_>(py_slice[0]), py::reinterpret_borrow<py::int_>(py_slice[1]),
                        py::reinterpret_borrow<py::int_>(py_slice[2]));
      } else if (py_slice[0].is_none() && py_slice[2].is_none()) {
        c_slice = Slice(py::reinterpret_borrow<py::int_>(py_slice[1]));
      } else if (!py_slice[0].is_none() && !py_slice[1].is_none()) {
        c_slice = Slice(py::reinterpret_borrow<py::int_>(py_slice[0]), py::reinterpret_borrow<py::int_>(py_slice[1]));
      }

      if (!c_slice.valid()) {
        THROW_IF_ERROR(Status(StatusCode::kUnexpectedError, __LINE__, __FILE__, "Wrong slice object"));
      }
      return std::make_shared<SliceOp>(c_slice);
    }));

  (void)py::enum_<RelationalOp>(*m, "RelationalOp", py::arithmetic())
    .value("EQ", RelationalOp::kEqual)
    .value("NE", RelationalOp::kNotEqual)
    .value("LT", RelationalOp::kLess)
    .value("LE", RelationalOp::kLessEqual)
    .value("GT", RelationalOp::kGreater)
    .value("GE", RelationalOp::kGreaterEqual)
    .export_values();

  (void)py::class_<MaskOp, TensorOp, std::shared_ptr<MaskOp>>(*m, "MaskOp",
                                                              "Tensor mask operation using relational comparator")
    .def(py::init<RelationalOp, std::shared_ptr<Tensor>, DataType>());

  (void)py::class_<DuplicateOp, TensorOp, std::shared_ptr<DuplicateOp>>(*m, "DuplicateOp", "Duplicate tensor.")
    .def(py::init<>());

  (void)py::class_<TruncateSequencePairOp, TensorOp, std::shared_ptr<TruncateSequencePairOp>>(
    *m, "TruncateSequencePairOp", "Tensor operation to truncate two tensors to a max_length")
    .def(py::init<int64_t>());

  (void)py::class_<ConcatenateOp, TensorOp, std::shared_ptr<ConcatenateOp>>(*m, "ConcatenateOp",
                                                                            "Tensor operation concatenate tensors.")
    .def(py::init<int8_t, std::shared_ptr<Tensor>, std::shared_ptr<Tensor>>(), py::arg("axis"),
         py::arg("prepend").none(true), py::arg("append").none(true));

  (void)py::class_<RandomRotationOp, TensorOp, std::shared_ptr<RandomRotationOp>>(
    *m, "RandomRotationOp",
    "Tensor operation to apply RandomRotation."
    "Takes a range for degrees and "
    "optional parameters for rotation center and image expand")
    .def(py::init<float, float, float, float, InterpolationMode, bool, uint8_t, uint8_t, uint8_t>(),
         py::arg("startDegree"), py::arg("endDegree"), py::arg("centerX") = RandomRotationOp::kDefCenterX,
         py::arg("centerY") = RandomRotationOp::kDefCenterY,
         py::arg("interpolation") = RandomRotationOp::kDefInterpolation,
         py::arg("expand") = RandomRotationOp::kDefExpand, py::arg("fillR") = RandomRotationOp::kDefFillR,
         py::arg("fillG") = RandomRotationOp::kDefFillG, py::arg("fillB") = RandomRotationOp::kDefFillB);

  (void)py::class_<PadEndOp, TensorOp, std::shared_ptr<PadEndOp>>(
    *m, "PadEndOp", "Tensor operation to pad end of tensor with a pad value.")
    .def(py::init<TensorShape, std::shared_ptr<Tensor>>());
}

void bindTensorOps3(py::module *m) {
  (void)py::class_<RandomCropAndResizeOp, TensorOp, std::shared_ptr<RandomCropAndResizeOp>>(
    *m, "RandomCropAndResizeOp",
    "Tensor operation to randomly crop an image and resize to a given size."
    "Takes output height and width and"
    "optional parameters for lower and upper bound for aspect ratio (h/w) and scale,"
    "interpolation mode, and max attempts to crop")
    .def(py::init<int32_t, int32_t, float, float, float, float, InterpolationMode, int32_t>(), py::arg("targetHeight"),
         py::arg("targetWidth"), py::arg("scaleLb") = RandomCropAndResizeOp::kDefScaleLb,
         py::arg("scaleUb") = RandomCropAndResizeOp::kDefScaleUb,
         py::arg("aspectLb") = RandomCropAndResizeOp::kDefAspectLb,
         py::arg("aspectUb") = RandomCropAndResizeOp::kDefAspectUb,
         py::arg("interpolation") = RandomCropAndResizeOp::kDefInterpolation,
         py::arg("maxIter") = RandomCropAndResizeOp::kDefMaxIter);

  (void)py::class_<RandomCropAndResizeWithBBoxOp, TensorOp, std::shared_ptr<RandomCropAndResizeWithBBoxOp>>(
    *m, "RandomCropAndResizeWithBBoxOp",
    "Tensor operation to randomly crop an image (with BBoxes) and resize to a given size."
    "Takes output height and width and"
    "optional parameters for lower and upper bound for aspect ratio (h/w) and scale,"
    "interpolation mode, and max attempts to crop")
    .def(py::init<int32_t, int32_t, float, float, float, float, InterpolationMode, int32_t>(), py::arg("targetHeight"),
         py::arg("targetWidth"), py::arg("scaleLb") = RandomCropAndResizeWithBBoxOp::kDefScaleLb,
         py::arg("scaleUb") = RandomCropAndResizeWithBBoxOp::kDefScaleUb,
         py::arg("aspectLb") = RandomCropAndResizeWithBBoxOp::kDefAspectLb,
         py::arg("aspectUb") = RandomCropAndResizeWithBBoxOp::kDefAspectUb,
         py::arg("interpolation") = RandomCropAndResizeWithBBoxOp::kDefInterpolation,
         py::arg("maxIter") = RandomCropAndResizeWithBBoxOp::kDefMaxIter);

  (void)py::class_<RandomColorAdjustOp, TensorOp, std::shared_ptr<RandomColorAdjustOp>>(
    *m, "RandomColorAdjustOp",
    "Tensor operation to adjust an image's color randomly."
    "Takes range for brightness, contrast, saturation, hue and")
    .def(py::init<float, float, float, float, float, float, float, float>(), py::arg("bright_factor_start"),
         py::arg("bright_factor_end"), py::arg("contrast_factor_start"), py::arg("contrast_factor_end"),
         py::arg("saturation_factor_start"), py::arg("saturation_factor_end"), py::arg("hue_factor_start"),
         py::arg("hue_factor_end"));

  (void)py::class_<RandomResizeOp, TensorOp, std::shared_ptr<RandomResizeOp>>(
    *m, "RandomResizeOp",
    "Tensor operation to resize an image using a randomly selected interpolation. Takes height and width.")
    .def(py::init<int32_t, int32_t>(), py::arg("targetHeight"),
         py::arg("targetWidth") = RandomResizeOp::kDefTargetWidth);

  (void)py::class_<CutOutOp, TensorOp, std::shared_ptr<CutOutOp>>(
    *m, "CutOutOp", "Tensor operation to randomly erase a portion of the image. Takes height and width.")
    .def(py::init<int32_t, int32_t, int32_t, bool, uint8_t, uint8_t, uint8_t>(), py::arg("boxHeight"),
         py::arg("boxWidth"), py::arg("numPatches"), py::arg("randomColor") = CutOutOp::kDefRandomColor,
         py::arg("fillR") = CutOutOp::kDefFillR, py::arg("fillG") = CutOutOp::kDefFillG,
         py::arg("fillB") = CutOutOp::kDefFillB);
}

void bindTensorOps4(py::module *m) {
  (void)py::class_<TypeCastOp, TensorOp, std::shared_ptr<TypeCastOp>>(
    *m, "TypeCastOp", "Tensor operator to type cast data to a specified type.")
    .def(py::init<DataType>(), py::arg("data_type"))
    .def(py::init<std::string>(), py::arg("data_type"));

  (void)py::class_<NoOp, TensorOp, std::shared_ptr<NoOp>>(*m, "NoOp",
                                                          "TensorOp that does nothing, for testing purposes only.")
    .def(py::init<>());

  (void)py::class_<ToFloat16Op, TensorOp, std::shared_ptr<ToFloat16Op>>(
    *m, "ToFloat16Op", py::dynamic_attr(), "Tensor operator to type cast float32 data to a float16 type.")
    .def(py::init<>());

  (void)py::class_<RandomCropDecodeResizeOp, TensorOp, std::shared_ptr<RandomCropDecodeResizeOp>>(
    *m, "RandomCropDecodeResizeOp", "equivalent to RandomCropAndResize but crops before decoding")
    .def(py::init<int32_t, int32_t, float, float, float, float, InterpolationMode, int32_t>(), py::arg("targetHeight"),
         py::arg("targetWidth"), py::arg("scaleLb") = RandomCropDecodeResizeOp::kDefScaleLb,
         py::arg("scaleUb") = RandomCropDecodeResizeOp::kDefScaleUb,
         py::arg("aspectLb") = RandomCropDecodeResizeOp::kDefAspectLb,
         py::arg("aspectUb") = RandomCropDecodeResizeOp::kDefAspectUb,
         py::arg("interpolation") = RandomCropDecodeResizeOp::kDefInterpolation,
         py::arg("maxIter") = RandomCropDecodeResizeOp::kDefMaxIter);

  (void)py::class_<PadOp, TensorOp, std::shared_ptr<PadOp>>(
    *m, "PadOp",
    "Pads image with specified color, default black, "
    "Takes amount to pad for top, bottom, left, right of image, boarder type and color")
    .def(py::init<int32_t, int32_t, int32_t, int32_t, BorderType, uint8_t, uint8_t, uint8_t>(), py::arg("padTop"),
         py::arg("padBottom"), py::arg("padLeft"), py::arg("padRight"), py::arg("borderTypes") = PadOp::kDefBorderType,
         py::arg("fillR") = PadOp::kDefFillR, py::arg("fillG") = PadOp::kDefFillG, py::arg("fillB") = PadOp::kDefFillB);
  (void)py::class_<ToNumberOp, TensorOp, std::shared_ptr<ToNumberOp>>(*m, "ToNumberOp",
                                                                      "TensorOp to convert strings to numbers.")
    .def(py::init<DataType>(), py::arg("data_type"))
    .def(py::init<std::string>(), py::arg("data_type"));
}

void bindTokenizerOps(py::module *m) {
  (void)py::class_<JiebaTokenizerOp, TensorOp, std::shared_ptr<JiebaTokenizerOp>>(*m, "JiebaTokenizerOp", "")
    .def(py::init<const std::string &, const std::string &, const JiebaMode &, const bool &>(), py::arg("hmm_path"),
         py::arg("mp_path"), py::arg("mode") = JiebaMode::kMix,
         py::arg("with_offsets") = JiebaTokenizerOp::kDefWithOffsets)
    .def("add_word",
         [](JiebaTokenizerOp &self, const std::string word, int freq) { THROW_IF_ERROR(self.AddWord(word, freq)); });
  (void)py::class_<UnicodeCharTokenizerOp, TensorOp, std::shared_ptr<UnicodeCharTokenizerOp>>(
    *m, "UnicodeCharTokenizerOp", "Tokenize a scalar tensor of UTF-8 string to Unicode characters.")
    .def(py::init<const bool &>(), py::arg("with_offsets") = UnicodeCharTokenizerOp::kDefWithOffsets);
  (void)py::class_<LookupOp, TensorOp, std::shared_ptr<LookupOp>>(*m, "LookupOp",
                                                                  "Tensor operation to LookUp each word.")
    .def(py::init([](std::shared_ptr<Vocab> vocab, const py::object &py_word) {
      if (vocab == nullptr) {
        THROW_IF_ERROR(Status(StatusCode::kUnexpectedError, "vocab object type is incorrect or null."));
      }
      if (py_word.is_none()) {
        return std::make_shared<LookupOp>(vocab, Vocab::kNoTokenExists);
      }
      std::string word = py::reinterpret_borrow<py::str>(py_word);
      WordIdType default_id = vocab->Lookup(word);
      if (default_id == Vocab::kNoTokenExists) {
        THROW_IF_ERROR(
          Status(StatusCode::kUnexpectedError, "default unknown token:" + word + " doesn't exist in vocab."));
      }
      return std::make_shared<LookupOp>(vocab, default_id);
    }));
  (void)py::class_<NgramOp, TensorOp, std::shared_ptr<NgramOp>>(*m, "NgramOp", "TensorOp performs ngram mapping.")
    .def(py::init<const std::vector<int32_t> &, int32_t, int32_t, const std::string &, const std::string &,
                  const std::string &>(),
         py::arg("ngrams"), py::arg("l_pad_len"), py::arg("r_pad_len"), py::arg("l_pad_token"), py::arg("r_pad_token"),
         py::arg("separator"));
  (void)py::class_<WordpieceTokenizerOp, TensorOp, std::shared_ptr<WordpieceTokenizerOp>>(
    *m, "WordpieceTokenizerOp", "Tokenize scalar token or 1-D tokens to subword tokens.")
    .def(
      py::init<const std::shared_ptr<Vocab> &, const std::string &, const int &, const std::string &, const bool &>(),
      py::arg("vocab"), py::arg("suffix_indicator") = std::string(WordpieceTokenizerOp::kDefSuffixIndicator),
      py::arg("max_bytes_per_token") = WordpieceTokenizerOp::kDefMaxBytesPerToken,
      py::arg("unknown_token") = std::string(WordpieceTokenizerOp::kDefUnknownToken),
      py::arg("with_offsets") = WordpieceTokenizerOp::kDefWithOffsets);
}

void bindDependIcuTokenizerOps(py::module *m) {
#ifdef ENABLE_ICU4C
  (void)py::class_<WhitespaceTokenizerOp, TensorOp, std::shared_ptr<WhitespaceTokenizerOp>>(
    *m, "WhitespaceTokenizerOp", "Tokenize a scalar tensor of UTF-8 string on ICU defined whitespaces.")
    .def(py::init<const bool &>(), py::arg("with_offsets") = WhitespaceTokenizerOp::kDefWithOffsets);
  (void)py::class_<UnicodeScriptTokenizerOp, TensorOp, std::shared_ptr<UnicodeScriptTokenizerOp>>(
    *m, "UnicodeScriptTokenizerOp", "Tokenize a scalar tensor of UTF-8 string on Unicode script boundaries.")
    .def(py::init<>())
    .def(py::init<const bool &, const bool &>(),
         py::arg("keep_whitespace") = UnicodeScriptTokenizerOp::kDefKeepWhitespace,
         py::arg("with_offsets") = UnicodeScriptTokenizerOp::kDefWithOffsets);
  (void)py::class_<CaseFoldOp, TensorOp, std::shared_ptr<CaseFoldOp>>(
    *m, "CaseFoldOp", "Apply case fold operation on utf-8 string tensor")
    .def(py::init<>());
  (void)py::class_<NormalizeUTF8Op, TensorOp, std::shared_ptr<NormalizeUTF8Op>>(
    *m, "NormalizeUTF8Op", "Apply normalize operation on utf-8 string tensor.")
    .def(py::init<>())
    .def(py::init<NormalizeForm>(), py::arg("normalize_form") = NormalizeUTF8Op::kDefNormalizeForm);
  (void)py::class_<RegexReplaceOp, TensorOp, std::shared_ptr<RegexReplaceOp>>(
    *m, "RegexReplaceOp", "Replace utf-8 string tensor with 'replace' according to regular expression 'pattern'.")
    .def(py::init<const std::string &, const std::string &, bool>(), py::arg("pattern"), py::arg("replace"),
         py::arg("replace_all"));
  (void)py::class_<RegexTokenizerOp, TensorOp, std::shared_ptr<RegexTokenizerOp>>(
    *m, "RegexTokenizerOp", "Tokenize a scalar tensor of UTF-8 string by regex expression pattern.")
    .def(py::init<const std::string &, const std::string &, const bool &>(), py::arg("delim_pattern"),
         py::arg("keep_delim_pattern"), py::arg("with_offsets") = RegexTokenizerOp::kDefWithOffsets);
  (void)py::class_<BasicTokenizerOp, TensorOp, std::shared_ptr<BasicTokenizerOp>>(
    *m, "BasicTokenizerOp", "Tokenize a scalar tensor of UTF-8 string by specific rules.")
    .def(py::init<const bool &, const bool &, const NormalizeForm &, const bool &, const bool &>(),
         py::arg("lower_case") = BasicTokenizerOp::kDefLowerCase,
         py::arg("keep_whitespace") = BasicTokenizerOp::kDefKeepWhitespace,
         py::arg("normalization_form") = BasicTokenizerOp::kDefNormalizationForm,
         py::arg("preserve_unused_token") = BasicTokenizerOp::kDefPreserveUnusedToken,
         py::arg("with_offsets") = BasicTokenizerOp::kDefWithOffsets);
  (void)py::class_<BertTokenizerOp, TensorOp, std::shared_ptr<BertTokenizerOp>>(*m, "BertTokenizerOp",
                                                                                "Tokenizer used for Bert text process.")
    .def(py::init<const std::shared_ptr<Vocab> &, const std::string &, const int &, const std::string &, const bool &,
                  const bool &, const NormalizeForm &, const bool &, const bool &>(),
         py::arg("vocab"), py::arg("suffix_indicator") = std::string(WordpieceTokenizerOp::kDefSuffixIndicator),
         py::arg("max_bytes_per_token") = WordpieceTokenizerOp::kDefMaxBytesPerToken,
         py::arg("unknown_token") = std::string(WordpieceTokenizerOp::kDefUnknownToken),
         py::arg("lower_case") = BasicTokenizerOp::kDefLowerCase,
         py::arg("keep_whitespace") = BasicTokenizerOp::kDefKeepWhitespace,
         py::arg("normalization_form") = BasicTokenizerOp::kDefNormalizationForm,
         py::arg("preserve_unused_token") = BasicTokenizerOp::kDefPreserveUnusedToken,
         py::arg("with_offsets") = WordpieceTokenizerOp::kDefWithOffsets);
#endif
}

void bindSamplerOps(py::module *m) {
  (void)py::class_<Sampler, std::shared_ptr<Sampler>>(*m, "Sampler")
    .def("set_num_rows", [](Sampler &self, int64_t rows) { THROW_IF_ERROR(self.SetNumRowsInDataset(rows)); })
    .def("set_num_samples", [](Sampler &self, int64_t samples) { THROW_IF_ERROR(self.SetNumSamples(samples)); })
    .def("initialize", [](Sampler &self) { THROW_IF_ERROR(self.InitSampler()); })
    .def("get_indices",
         [](Sampler &self) {
           py::array ret;
           THROW_IF_ERROR(self.GetAllIdsThenReset(&ret));
           return ret;
         })
    .def("add_child",
         [](std::shared_ptr<Sampler> self, std::shared_ptr<Sampler> child) { THROW_IF_ERROR(self->AddChild(child)); });

  (void)py::class_<mindrecord::ShardOperator, std::shared_ptr<mindrecord::ShardOperator>>(*m, "ShardOperator")
    .def("add_child", [](std::shared_ptr<mindrecord::ShardOperator> self,
                         std::shared_ptr<mindrecord::ShardOperator> child) { self->SetChildOp(child); });

  (void)py::class_<DistributedSampler, Sampler, std::shared_ptr<DistributedSampler>>(*m, "DistributedSampler")
    .def(py::init<int64_t, int64_t, int64_t, bool, uint32_t>());

  (void)py::class_<PKSampler, Sampler, std::shared_ptr<PKSampler>>(*m, "PKSampler")
    .def(py::init<int64_t, int64_t, bool>());

  (void)py::class_<RandomSampler, Sampler, std::shared_ptr<RandomSampler>>(*m, "RandomSampler")
    .def(py::init<int64_t, bool, bool>());

  (void)py::class_<SequentialSampler, Sampler, std::shared_ptr<SequentialSampler>>(*m, "SequentialSampler")
    .def(py::init<int64_t, int64_t>());

  (void)py::class_<SubsetRandomSampler, Sampler, std::shared_ptr<SubsetRandomSampler>>(*m, "SubsetRandomSampler")
    .def(py::init<int64_t, std::vector<int64_t>>());

  (void)py::class_<mindrecord::ShardSample, mindrecord::ShardOperator, std::shared_ptr<mindrecord::ShardSample>>(
    *m, "MindrecordSubsetRandomSampler")
    .def(py::init<std::vector<int64_t>, uint32_t>(), py::arg("indices"), py::arg("seed") = GetSeed());

  (void)py::class_<mindrecord::ShardPkSample, mindrecord::ShardOperator, std::shared_ptr<mindrecord::ShardPkSample>>(
    *m, "MindrecordPkSampler")
    .def(py::init([](int64_t kVal, std::string kColumn, bool shuffle) {
      if (shuffle == true) {
        return std::make_shared<mindrecord::ShardPkSample>(kColumn, kVal, std::numeric_limits<int64_t>::max(),
                                                           GetSeed());
      } else {
        return std::make_shared<mindrecord::ShardPkSample>(kColumn, kVal);
      }
    }));

  (void)py::class_<mindrecord::ShardDistributedSample, mindrecord::ShardSample,
                   std::shared_ptr<mindrecord::ShardDistributedSample>>(*m, "MindrecordDistributedSampler")
    .def(py::init<int64_t, int64_t, bool, uint32_t>());

  (void)py::class_<mindrecord::ShardShuffle, mindrecord::ShardOperator, std::shared_ptr<mindrecord::ShardShuffle>>(
    *m, "MindrecordRandomSampler")
    .def(py::init([](int64_t num_samples, bool replacement, bool reshuffle_each_epoch) {
      return std::make_shared<mindrecord::ShardShuffle>(GetSeed(), num_samples, replacement, reshuffle_each_epoch);
    }));

  (void)py::class_<mindrecord::ShardSequentialSample, mindrecord::ShardSample,
                   std::shared_ptr<mindrecord::ShardSequentialSample>>(*m, "MindrecordSequentialSampler")
    .def(py::init([](int num_samples, int start_index) {
      return std::make_shared<mindrecord::ShardSequentialSample>(num_samples, start_index);
    }));

  (void)py::class_<WeightedRandomSampler, Sampler, std::shared_ptr<WeightedRandomSampler>>(*m, "WeightedRandomSampler")
    .def(py::init<int64_t, std::vector<double>, bool>());

  (void)py::class_<PythonSampler, Sampler, std::shared_ptr<PythonSampler>>(*m, "PythonSampler")
    .def(py::init<int64_t, py::object>());
}

void bindInfoObjects(py::module *m) {
  (void)py::class_<BatchOp::CBatchInfo>(*m, "CBatchInfo")
    .def(py::init<int64_t, int64_t, int64_t>())
    .def("get_epoch_num", &BatchOp::CBatchInfo::get_epoch_num)
    .def("get_batch_num", &BatchOp::CBatchInfo::get_batch_num);
}

void bindCacheClient(py::module *m) {
  (void)py::class_<CacheClient, std::shared_ptr<CacheClient>>(*m, "CacheClient")
    .def(py::init<uint32_t, uint64_t, bool>());
}

void bindVocabObjects(py::module *m) {
  (void)py::class_<Vocab, std::shared_ptr<Vocab>>(*m, "Vocab")
    .def(py::init<>())
    .def_static("from_list",
                [](const py::list &words, const py::list &special_tokens, bool special_first) {
                  std::shared_ptr<Vocab> v;
                  THROW_IF_ERROR(Vocab::BuildFromPyList(words, special_tokens, special_first, &v));
                  return v;
                })
    .def_static("from_file",
                [](const std::string &path, const std::string &dlm, int32_t vocab_size, const py::list &special_tokens,
                   bool special_first) {
                  std::shared_ptr<Vocab> v;
                  THROW_IF_ERROR(Vocab::BuildFromFile(path, dlm, vocab_size, special_tokens, special_first, &v));
                  return v;
                })
    .def_static("from_dict", [](const py::dict &words) {
      std::shared_ptr<Vocab> v;
      THROW_IF_ERROR(Vocab::BuildFromPyDict(words, &v));
      return v;
    });
}

void bindGraphData(py::module *m) {
  (void)py::class_<gnn::Graph, std::shared_ptr<gnn::Graph>>(*m, "Graph")
    .def(py::init([](std::string dataset_file, int32_t num_workers) {
      std::shared_ptr<gnn::Graph> g_out = std::make_shared<gnn::Graph>(dataset_file, num_workers);
      THROW_IF_ERROR(g_out->Init());
      return g_out;
    }))
    .def("get_all_nodes",
         [](gnn::Graph &g, gnn::NodeType node_type) {
           std::shared_ptr<Tensor> out;
           THROW_IF_ERROR(g.GetAllNodes(node_type, &out));
           return out;
         })
    .def("get_all_edges",
         [](gnn::Graph &g, gnn::EdgeType edge_type) {
           std::shared_ptr<Tensor> out;
           THROW_IF_ERROR(g.GetAllEdges(edge_type, &out));
           return out;
         })
    .def("get_nodes_from_edges",
         [](gnn::Graph &g, std::vector<gnn::NodeIdType> edge_list) {
           std::shared_ptr<Tensor> out;
           THROW_IF_ERROR(g.GetNodesFromEdges(edge_list, &out));
           return out;
         })
    .def("get_all_neighbors",
         [](gnn::Graph &g, std::vector<gnn::NodeIdType> node_list, gnn::NodeType neighbor_type) {
           std::shared_ptr<Tensor> out;
           THROW_IF_ERROR(g.GetAllNeighbors(node_list, neighbor_type, &out));
           return out;
         })
    .def("get_sampled_neighbors",
         [](gnn::Graph &g, std::vector<gnn::NodeIdType> node_list, std::vector<gnn::NodeIdType> neighbor_nums,
            std::vector<gnn::NodeType> neighbor_types) {
           std::shared_ptr<Tensor> out;
           THROW_IF_ERROR(g.GetSampledNeighbors(node_list, neighbor_nums, neighbor_types, &out));
           return out;
         })
    .def("get_neg_sampled_neighbors",
         [](gnn::Graph &g, std::vector<gnn::NodeIdType> node_list, gnn::NodeIdType neighbor_num,
            gnn::NodeType neg_neighbor_type) {
           std::shared_ptr<Tensor> out;
           THROW_IF_ERROR(g.GetNegSampledNeighbors(node_list, neighbor_num, neg_neighbor_type, &out));
           return out;
         })
    .def("get_node_feature",
         [](gnn::Graph &g, std::shared_ptr<Tensor> node_list, std::vector<gnn::FeatureType> feature_types) {
           TensorRow out;
           THROW_IF_ERROR(g.GetNodeFeature(node_list, feature_types, &out));
           return out.getRow();
         })
    .def("get_edge_feature",
         [](gnn::Graph &g, std::shared_ptr<Tensor> edge_list, std::vector<gnn::FeatureType> feature_types) {
           TensorRow out;
           THROW_IF_ERROR(g.GetEdgeFeature(edge_list, feature_types, &out));
           return out.getRow();
         })
    .def("graph_info",
         [](gnn::Graph &g) {
           py::dict out;
           THROW_IF_ERROR(g.GraphInfo(&out));
           return out;
         })
    .def("random_walk", [](gnn::Graph &g, std::vector<gnn::NodeIdType> node_list, std::vector<gnn::NodeType> meta_path,
                           float step_home_param, float step_away_param, gnn::NodeIdType default_node) {
      std::shared_ptr<Tensor> out;
      THROW_IF_ERROR(g.RandomWalk(node_list, meta_path, step_home_param, step_away_param, default_node, &out));
      return out;
    });
}

// This is where we externalize the C logic as python modules
PYBIND11_MODULE(_c_dataengine, m) {
  m.doc() = "pybind11 for _c_dataengine";
  (void)py::class_<DatasetOp, std::shared_ptr<DatasetOp>>(m, "DatasetOp");

  (void)py::enum_<OpName>(m, "OpName", py::arithmetic())
    .value("SHUFFLE", OpName::kShuffle)
    .value("BATCH", OpName::kBatch)
    .value("BUCKETBATCH", OpName::kBucketBatch)
    .value("BARRIER", OpName::kBarrier)
    .value("MINDRECORD", OpName::kMindrecord)
    .value("CACHE", OpName::kCache)
    .value("REPEAT", OpName::kRepeat)
    .value("SKIP", OpName::kSkip)
    .value("TAKE", OpName::kTake)
    .value("ZIP", OpName::kZip)
    .value("CONCAT", OpName::kConcat)
    .value("MAP", OpName::kMap)
    .value("FILTER", OpName::kFilter)
    .value("DEVICEQUEUE", OpName::kDeviceQueue)
    .value("GENERATOR", OpName::kGenerator)
    .export_values()
    .value("RENAME", OpName::kRename)
    .value("TFREADER", OpName::kTfReader)
    .value("PROJECT", OpName::kProject)
    .value("IMAGEFOLDER", OpName::kImageFolder)
    .value("MNIST", OpName::kMnist)
    .value("MANIFEST", OpName::kManifest)
    .value("VOC", OpName::kVoc)
    .value("COCO", OpName::kCoco)
    .value("CIFAR10", OpName::kCifar10)
    .value("CIFAR100", OpName::kCifar100)
    .value("RANDOMDATA", OpName::kRandomData)
    .value("BUILDVOCAB", OpName::kBuildVocab)
    .value("CELEBA", OpName::kCelebA)
    .value("TEXTFILE", OpName::kTextFile)
    .value("CLUE", OpName::kClue);

  (void)py::enum_<JiebaMode>(m, "JiebaMode", py::arithmetic())
    .value("DE_JIEBA_MIX", JiebaMode::kMix)
    .value("DE_JIEBA_MP", JiebaMode::kMp)
    .value("DE_JIEBA_HMM", JiebaMode::kHmm)
    .export_values();

#ifdef ENABLE_ICU4C
  (void)py::enum_<NormalizeForm>(m, "NormalizeForm", py::arithmetic())
    .value("DE_NORMALIZE_NONE", NormalizeForm::kNone)
    .value("DE_NORMALIZE_NFC", NormalizeForm::kNfc)
    .value("DE_NORMALIZE_NFKC", NormalizeForm::kNfkc)
    .value("DE_NORMALIZE_NFD", NormalizeForm::kNfd)
    .value("DE_NORMALIZE_NFKD", NormalizeForm::kNfkd)
    .export_values();
#endif

  (void)py::enum_<InterpolationMode>(m, "InterpolationMode", py::arithmetic())
    .value("DE_INTER_LINEAR", InterpolationMode::kLinear)
    .value("DE_INTER_CUBIC", InterpolationMode::kCubic)
    .value("DE_INTER_AREA", InterpolationMode::kArea)
    .value("DE_INTER_NEAREST_NEIGHBOUR", InterpolationMode::kNearestNeighbour)
    .export_values();

  (void)py::enum_<BorderType>(m, "BorderType", py::arithmetic())
    .value("DE_BORDER_CONSTANT", BorderType::kConstant)
    .value("DE_BORDER_EDGE", BorderType::kEdge)
    .value("DE_BORDER_REFLECT", BorderType::kReflect)
    .value("DE_BORDER_SYMMETRIC", BorderType::kSymmetric)
    .export_values();
  bindDEPipeline(&m);
  bindTensor(&m);
  bindTensorOps1(&m);
  bindTensorOps2(&m);
  bindTensorOps3(&m);
  bindTensorOps4(&m);
  bindTokenizerOps(&m);
  bindSamplerOps(&m);
  bindDatasetOps(&m);
  bindInfoObjects(&m);
  bindCacheClient(&m);
  bindVocabObjects(&m);
  bindGraphData(&m);
  bindDependIcuTokenizerOps(&m);
}
}  // namespace dataset
}  // namespace mindspore
