#include "transfer_dialog.h"
#include "list_of_job_options.h"
#include "utils.h"

TransferDialog::TransferDialog(bool isDownload, bool isDrop,
                               const QString &remote, const QDir &path,
                               bool isFolder, QWidget *parent, JobOptions *task,
                               bool editMode)
    : QDialog(parent), mIsDownload(isDownload), mIsFolder(isFolder),
      mIsEditMode(editMode), mJobOptions(task) {
  ui.setupUi(this);
  resize(0, 0);
  setWindowTitle(isDownload ? "Download" : "Upload");

  QStyle *style = qApp->style();
  ui.buttonSourceFile->setIcon(style->standardIcon(QStyle::SP_FileIcon));
  ui.buttonSourceFolder->setIcon(style->standardIcon(QStyle::SP_DirIcon));
  ui.buttonDest->setIcon(style->standardIcon(QStyle::SP_DirIcon));

  ui.buttonDefaultSource->setIcon(style->standardIcon(QStyle::SP_DirHomeIcon));
  ui.buttonDefaultDest->setIcon(style->standardIcon(QStyle::SP_DirHomeIcon));

  if (!mIsEditMode) {
    QPushButton *dryRun =
        ui.buttonBox->addButton("&Dry run", QDialogButtonBox::AcceptRole);
    ui.buttonBox->addButton("&Run", QDialogButtonBox::AcceptRole);
    QObject::connect(dryRun, &QPushButton::clicked, this,
                     [=]() { mDryRun = true; });
  }

  QPushButton *saveTask = ui.buttonBox->addButton(
      "&Save task", QDialogButtonBox::ButtonRole::ActionRole);

  QObject::connect(
      ui.buttonBox->button(QDialogButtonBox::RestoreDefaults),
      &QPushButton::clicked, this, [=]() {
        ui.cbSyncDelete->setCurrentIndex(0);
        // set combobox tooltips
        ui.cbSyncDelete->setItemData(0, "--delete-during", Qt::ToolTipRole);
        ui.cbSyncDelete->setItemData(1, "--delete-after", Qt::ToolTipRole);
        ui.cbSyncDelete->setItemData(2, "--delete-before", Qt::ToolTipRole);
        ui.checkSkipNewer->setChecked(false);
        ui.checkSkipNewer->setChecked(false);
        ui.checkSkipExisting->setChecked(false);
        ui.checkCompare->setChecked(true);
        ui.cbCompare->setCurrentIndex(0);
        // set combobox tooltips
        ui.cbCompare->setItemData(0, "default", Qt::ToolTipRole);
        ui.cbCompare->setItemData(1, "--checksum", Qt::ToolTipRole);
        ui.cbCompare->setItemData(2, "--ignore-size", Qt::ToolTipRole);
        ui.cbCompare->setItemData(3, "--size-only", Qt::ToolTipRole);
        ui.cbCompare->setItemData(4, "--checksum --ignore-size",
                                  Qt::ToolTipRole);
        //      ui.checkVerbose->setChecked(false);
        ui.checkSameFilesystem->setChecked(false);
        ui.checkDontUpdateModified->setChecked(false);
        ui.spinTransfers->setValue(4);
        ui.spinCheckers->setValue(8);
        ui.textBandwidth->clear();
        ui.textMinSize->clear();
        ui.textMinAge->clear();
        ui.textMaxAge->clear();
        ui.spinMaxDepth->setValue(0);
        ui.spinConnectTimeout->setValue(60);
        ui.spinIdleTimeout->setValue(300);
        ui.spinRetries->setValue(3);
        ui.spinLowLevelRetries->setValue(10);
        ui.checkDeleteExcluded->setChecked(false);
        ui.textExclude->clear();
        auto settings = GetSettings();
        if (isDownload) {
          // download
          ui.textExtra->setText(
              settings->value("Settings/defaultDownloadOptions").toString());
        } else {
          // upload
          ui.textExtra->setText(
              settings->value("Settings/defaultUploadOptions").toString());
        }
      });

  ui.buttonBox->button(QDialogButtonBox::RestoreDefaults)->click();

  QObject::connect(saveTask, &QPushButton::clicked, this, [=]() {
    // validate before saving task...
    if (ui.textDescription->text().isEmpty()) {
      QMessageBox::warning(this, "Warning",
                           "Please enter task description to Save!");
      ui.textDescription->setFocus(Qt::FocusReason::OtherFocusReason);
      return;
    }
    // even though the below does not match the condition on the Run buttons
    // it SEEMS like blanking either one would be a problem, right?
    if (ui.textDest->text().isEmpty() || ui.textSource->text().isEmpty()) {
      QMessageBox::warning(this, "Error",
                           "Invalid Task, source and destination required!");
      return;
    }
    JobOptions *jobo = getJobOptions();
    ListOfJobOptions::getInstance()->Persist(jobo);
    // always close on save
    // if (mIsEditMode)
    this->close();
  });

  QObject::connect(ui.buttonBox, &QDialogButtonBox::accepted, this,
                   &QDialog::accept);
  QObject::connect(ui.buttonBox, &QDialogButtonBox::rejected, this,
                   &QDialog::reject);

  QObject::connect(ui.buttonSourceFile, &QToolButton::clicked, this, [=]() {
    QString file = QFileDialog::getOpenFileName(this, "Choose file to upload");
    if (!file.isEmpty()) {
      ui.textSource->setText(QDir::toNativeSeparators(file));
      ui.textDest->setText(remote + ":" + path.path());
    }
  });

  QObject::connect(ui.buttonSourceFolder, &QToolButton::clicked, this, [=]() {
    auto settings = GetSettings();
    QString last_used_source_folder =
        (settings->value("Settings/lastUsedSourceFolder").toString());
    QString folder = QFileDialog::getExistingDirectory(
        this, "Choose folder to upload", last_used_source_folder,
        QFileDialog::ShowDirsOnly);
    if (!folder.isEmpty()) {
      // store new folder in lastUsedSourceFolder
      settings->setValue("Settings/lastUsedSourceFolder", folder);
      ui.textSource->setText(QDir::toNativeSeparators(folder));
      ui.textDest->setText(remote + ":" +
          path.filePath(QFileInfo(folder).fileName()));
    }
  });

  QObject::connect(ui.buttonDefaultSource, &QToolButton::clicked, this, [=]() {
    auto settings = GetSettings();
    QString default_folder =
        (settings->value("Settings/defaultUploadDir").toString());
    // store default folder in lastUsedSourceFolder
    settings->setValue("Settings/lastUsedSourceFolder", default_folder);
    ui.textSource->setText(QDir::toNativeSeparators(default_folder));
    if (!default_folder.isEmpty()) {
      ui.textDest->setText(remote + ":" +
          path.filePath(QFileInfo(default_folder).fileName()));
    } else {
      ui.textDest->setText(remote + ":" + path.path());
    };
  });

  QObject::connect(ui.buttonDest, &QToolButton::clicked, this, [=]() {
    auto settings = GetSettings();
    QString last_used_dest_folder =
        (settings->value("Settings/lastUsedDestFolder").toString());
    QString folder = QFileDialog::getExistingDirectory(
        this, "Choose destination folder", last_used_dest_folder,
        QFileDialog::ShowDirsOnly);
    if (!folder.isEmpty()) {
      // store new folder in lastUsedDestFolder
      settings->setValue("Settings/lastUsedDestFolder", folder);
      if (isFolder) {
        ui.textDest->setText(
            QDir::toNativeSeparators(folder + "/" + path.dirName()));
      } else {
        ui.textDest->setText(QDir::toNativeSeparators(folder));
      }
    }
  });

  QObject::connect(ui.buttonDefaultDest, &QToolButton::clicked, this, [=]() {
    auto settings = GetSettings();
    QString default_folder =
        (settings->value("Settings/defaultDownloadDir").toString());
    // store default_folder in lastUsedDestFolder
    settings->setValue("Settings/lastUsedDestFolder", default_folder);
    if (!default_folder.isEmpty()) {
      if (isFolder) {
        ui.textDest->setText(
            QDir::toNativeSeparators(default_folder + "/" + path.dirName()));
      } else {
        ui.textDest->setText(QDir::toNativeSeparators(default_folder));
      }
    } else {
      ui.textDest->setText("");
    };
  });

  auto settings = GetSettings();
  settings->beginGroup("Transfer");
  ReadSettings(settings.get(), this);
  settings->endGroup();

  ui.buttonSourceFile->setVisible(!isDownload);
  ui.buttonSourceFolder->setVisible(!isDownload);
  ui.buttonDefaultSource->setVisible(!isDownload);

  ui.buttonDest->setVisible(isDownload);
  ui.buttonDefaultDest->setVisible(isDownload);

  // Info only - should not be edited
  // would be nice to display it only for Google Drive - todo
  ui.checkisDriveSharedWithMe->setDisabled(true);

  ui.checkisDriveSharedWithMe->setChecked(
      settings->value("Settings/driveShared", false).toBool());
  // always clear for new jobs
  ui.textDescription->clear();

  if (mIsEditMode && mJobOptions != nullptr) {
    // it's not really valid for only one of these things to be true.
    // when operating on an existing instance i.e. a saved task,
    // changing the dest or src seems to have problems so we
    // will not allow it.  simple enough, and better, to make a
    // new task for different pairings anyway.  that will make
    // a lot more sense when/if scheduling and history are added...
    ui.buttonSourceFile->setVisible(false);
    ui.buttonSourceFolder->setVisible(false);
    ui.buttonDefaultSource->setVisible(false);

    ui.buttonDest->setVisible(false);
    ui.buttonDefaultDest->setVisible(false);

    ui.textDest->setDisabled(true);
    ui.textSource->setDisabled(true);
    putJobOptions();
  } else {

    // set source and destination using defaults
    if (isDownload) {
      // download
      ui.textExtra->setText(
          settings->value("Settings/defaultDownloadOptions").toString());
      ui.textSource->setText(remote + ":" + path.path());
      QString folder;
      QString default_folder =
          (settings->value("Settings/defaultDownloadDir").toString());
      QString last_used_dest_folder =
          (settings->value("Settings/lastUsedDestFolder").toString());

      if (last_used_dest_folder.isEmpty()) {
        folder = default_folder;
      } else {
        folder = last_used_dest_folder;
      };

      if (!folder.isEmpty()) {
        if (isFolder) {
          ui.textDest->setText(
              QDir::toNativeSeparators(folder + "/" + path.dirName()));
        } else {
          ui.textDest->setText(QDir::toNativeSeparators(folder));
        }
      }

    } else {
      // upload
      ui.textExtra->setText(
          settings->value("Settings/defaultUploadOptions").toString());
      QString folder;
      QString default_folder =
          (settings->value("Settings/defaultUploadDir").toString());
      QString last_used_source_folder =
          (settings->value("Settings/lastUsedSourceFolder").toString());

      if (last_used_source_folder.isEmpty()) {
        folder = default_folder;
      } else {
        folder = last_used_source_folder;
      };

      // if upload initiated from drag and drop we dont use default upload
      // folder
      if (!isDrop) {
        ui.textSource->setText(QDir::toNativeSeparators(folder));
        if (!folder.isEmpty()) {
          ui.textDest->setText(remote + ":" +
              path.filePath(QFileInfo(folder).fileName()));
        } else {
          ui.textDest->setText(remote + ":" + path.path());
        }
      } else {
        // when dropping to root folder
        if (path.path() == ".") {
          ui.textDest->setText(remote + ":");
        } else {
          ui.textDest->setText(remote + ":" + path.path());
        }
      };
    };
  }
}

TransferDialog::~TransferDialog() {
  if (result() == QDialog::Accepted) {
    auto settings = GetSettings();
    settings->beginGroup("Transfer");
    WriteSettings(settings.get(), this);
    settings->remove("textSource");
    settings->remove("textDest");
    settings->endGroup();
  }
}

void TransferDialog::setSource(const QString &path) {
  ui.textSource->setText(QDir::toNativeSeparators(path));
}

QString TransferDialog::getMode() const {
  if (ui.rbCopy->isChecked()) {
    return "Copy";
  } else if (ui.rbMove->isChecked()) {
    return "Move";
  } else if (ui.rbSync->isChecked()) {
    return "Sync";
  }

  return QString();
}

QString TransferDialog::getSource() const { return ui.textSource->text(); }

QString TransferDialog::getDest() const { return ui.textDest->text(); }

QStringList TransferDialog::getOptions() {
  JobOptions *jobo = getJobOptions();
  QStringList newWay = jobo->getOptions();
  return newWay;
}

/*
 * Apply the displayed/edited values on the UI to the
 * JobOptions object.
 *
 * This needs to be edited whenever options are added or changed.
 */
JobOptions *TransferDialog::getJobOptions() {
  if (mJobOptions == nullptr)
    mJobOptions = new JobOptions(mIsDownload);

  if (ui.rbCopy->isChecked()) {
    mJobOptions->operation = JobOptions::Copy;
    mJobOptions->sync = false;
  } else if (ui.rbMove->isChecked()) {
    mJobOptions->operation = JobOptions::Move;
    mJobOptions->sync = false;
  } else if (ui.rbSync->isChecked()) {
    mJobOptions->operation = JobOptions::Sync;
  }

  mJobOptions->dryRun = mDryRun;;

  if (ui.rbSync->isChecked()) {
    mJobOptions->sync = true;
    switch (ui.cbSyncDelete->currentIndex()) {
    case 0:
      mJobOptions->syncTiming = JobOptions::During;
      break;
    case 1:
      mJobOptions->syncTiming = JobOptions::After;
      break;
    case 2:
      mJobOptions->syncTiming = JobOptions::Before;
      break;
    }
  }

  mJobOptions->skipNewer = ui.checkSkipNewer->isChecked();
  mJobOptions->skipExisting = ui.checkSkipExisting->isChecked();

  if (ui.checkCompare->isChecked()) {
    mJobOptions->compare = true;
    switch (ui.cbCompare->currentIndex()) {
    case 0:
      mJobOptions->compareOption = JobOptions::SizeAndModTime;
      break;
    case 1:
      mJobOptions->compareOption = JobOptions::Checksum;
      break;
    case 2:
      mJobOptions->compareOption = JobOptions::IgnoreSize;
      break;
    case 3:
      mJobOptions->compareOption = JobOptions::SizeOnly;
      break;
    case 4:
      mJobOptions->compareOption = JobOptions::ChecksumIgnoreSize;
      break;
    }
  } else {
    mJobOptions->compare = false;
  };

  //    mJobOptions->verbose = ui.checkVerbose->isChecked();
  mJobOptions->sameFilesystem = ui.checkSameFilesystem->isChecked();
  mJobOptions->dontUpdateModified = ui.checkDontUpdateModified->isChecked();

  mJobOptions->transfers = ui.spinTransfers->text();
  mJobOptions->checkers = ui.spinCheckers->text();
  mJobOptions->bandwidth = ui.textBandwidth->text();
  mJobOptions->minSize = ui.textMinSize->text();
  mJobOptions->minAge = ui.textMinAge->text();
  mJobOptions->maxAge = ui.textMaxAge->text();
  mJobOptions->maxDepth = ui.spinMaxDepth->value();

  mJobOptions->connectTimeout = ui.spinConnectTimeout->text();
  mJobOptions->idleTimeout = ui.spinIdleTimeout->text();
  mJobOptions->retries = ui.spinRetries->text();
  mJobOptions->lowLevelRetries = ui.spinLowLevelRetries->text();
  mJobOptions->deleteExcluded = ui.checkDeleteExcluded->isChecked();

  mJobOptions->excluded = ui.textExclude->toPlainText().trimmed();
  mJobOptions->extra = ui.textExtra->text().trimmed();

  mJobOptions->source = ui.textSource->text();
  mJobOptions->dest = ui.textDest->text();
  mJobOptions->isFolder = mIsFolder;

  mJobOptions->description = ui.textDescription->text();
  //   auto settings = GetSettings();
  //   mJobOptions->DriveSharedWithMe = settings->value("Settings/driveShared",
  //   false).toBool();

  if (mIsEditMode)
    mJobOptions->DriveSharedWithMe = ui.checkisDriveSharedWithMe->isChecked();
  else {
    auto settings = GetSettings();
    mJobOptions->DriveSharedWithMe =
        settings->value("Settings/driveShared", false).toBool();
  };

  return mJobOptions;
}

/*
 * Apply the JobOptions object to the displayed widget values.
 *
 * It could be "better" to use a two-way binding mechanism, but
 * if used that should be global to the app; and anyway doing
 * it this old primitive way makes it easier when the user wants
 * to not save changes...
 */
void TransferDialog::putJobOptions() {
  ui.rbCopy->setChecked(mJobOptions->operation == JobOptions::Copy);
  ui.rbMove->setChecked(mJobOptions->operation == JobOptions::Move);
  ui.rbSync->setChecked(mJobOptions->operation == JobOptions::Sync);

  mDryRun = mJobOptions->dryRun;
  ui.rbSync->setChecked(mJobOptions->sync);

  ui.cbSyncDelete->setCurrentIndex((int) mJobOptions->syncTiming);
  // set combobox tooltips
  ui.cbSyncDelete->setItemData(0, "--delete-during", Qt::ToolTipRole);
  ui.cbSyncDelete->setItemData(1, "--delete-after", Qt::ToolTipRole);
  ui.cbSyncDelete->setItemData(2, "--delete-before", Qt::ToolTipRole);

  ui.checkSkipNewer->setChecked(mJobOptions->skipNewer);
  ui.checkSkipExisting->setChecked(mJobOptions->skipExisting);

  ui.checkCompare->setChecked(mJobOptions->compare);

  ui.cbCompare->setCurrentIndex(mJobOptions->compareOption);
  // set combobox tooltips
  ui.cbCompare->setItemData(0, "default", Qt::ToolTipRole);
  ui.cbCompare->setItemData(1, "--checksum", Qt::ToolTipRole);
  ui.cbCompare->setItemData(2, "--ignore-size", Qt::ToolTipRole);
  ui.cbCompare->setItemData(3, "--size-only", Qt::ToolTipRole);
  ui.cbCompare->setItemData(4, "--checksum --ignore-size", Qt::ToolTipRole);
  // ui.checkVerbose->setChecked(mJobOptions->verbose);
  ui.checkSameFilesystem->setChecked(mJobOptions->sameFilesystem);
  ui.checkDontUpdateModified->setChecked(mJobOptions->dontUpdateModified);

  ui.spinTransfers->setValue(mJobOptions->transfers.toInt());
  ui.spinCheckers->setValue(mJobOptions->checkers.toInt());
  ui.textBandwidth->setText(mJobOptions->bandwidth);
  ui.textMinSize->setText(mJobOptions->minSize);
  ui.textMinAge->setText(mJobOptions->minAge);
  ui.textMaxAge->setText(mJobOptions->maxAge);
  ui.spinMaxDepth->setValue(mJobOptions->maxDepth);

  ui.spinConnectTimeout->setValue(mJobOptions->connectTimeout.toInt());
  ui.spinIdleTimeout->setValue(mJobOptions->idleTimeout.toInt());
  ui.spinRetries->setValue(mJobOptions->retries.toInt());
  ui.spinLowLevelRetries->setValue(mJobOptions->lowLevelRetries.toInt());
  ui.checkDeleteExcluded->setChecked(mJobOptions->deleteExcluded);

  ui.textExclude->setPlainText(mJobOptions->excluded);
  ui.textExtra->setText(mJobOptions->extra);

  ui.textSource->setText(mJobOptions->source);
  ui.textDest->setText(mJobOptions->dest);
  ui.textDescription->setText(mJobOptions->description);
  // DDBB
  ui.checkisDriveSharedWithMe->setChecked(mJobOptions->DriveSharedWithMe);
}

void TransferDialog::done(int r) {
  if (r == QDialog::Accepted) {
    if (mIsDownload) {
      if (ui.textDest->text().isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please enter destination!");
        return;
      }
    } else {
      if (ui.textSource->text().isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please enter source!");
        return;
      }
    }
  }
  QDialog::done(r);
}
