TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += lib.pro

!android:!ios:!qmllive_no_bench {
  SUBDIRS += bench \
             previewGenerator
}

!qmllive_no_runtime {
  SUBDIRS += runtime
}
