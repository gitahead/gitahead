//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef MENUBAR
#define MENUBAR

#include <QMenuBar>

class History;
class RepoView;

class MenuBar : public QMenuBar
{
  Q_OBJECT

public:
  MenuBar(QWidget *parent = nullptr);

  void update();
  void updateFile();
  void updateSave();
  void updateUndoRedo();
  void updateCutCopyPaste();
  void updateSelectAll();
  void updateFind();
  void updateView();
  void updateRepository();
  void updateRemote();
  void updateBranch();
  void updateSubmodules();
  void updateStash();
  void updateHistory();
  void updateWindow();

  static void setDebugMenuVisible(bool show);
  static MenuBar *instance(QWidget *widget);

private:
  QWidget *window() const;
  RepoView *view() const;

  // File
  QAction *mClose;
  QAction *mSave;

  // Edit
  QAction *mUndo;
  QAction *mRedo;
  QAction *mCut;
  QAction *mCopy;
  QAction *mPaste;
  QAction *mSelectAll;

  QAction *mFind;
  QAction *mFindNext;
  QAction *mFindPrevious;
  QAction *mFindSelection;

  // View
  QAction *mRefresh;
  QAction *mToggleLog;
  QAction *mToggleView;

  // Repository
  QAction *mConfigureRepository;
  QAction *mCommit;
  QAction *mAmendCommit;
  QAction *mLfsUnlock;
  QAction *mLfsInitialize;

  // Remote
  QAction *mConfigureRemotes;
  QAction *mFetch;
  QAction *mPull;
  QAction *mPush;
  QAction *mFetchFrom;
  QAction *mPullFrom;
  QAction *mPushTo;

  // Branch
  QAction *mConfigureBranches;
  QAction *mNewBranch;
  QAction *mCheckoutCurrent;
  QAction *mCheckout;
  QAction *mMerge;
  QAction *mRebase;
  QAction *mAbort;

  // Submodule
  QAction *mConfigureSubmodules;
  QAction *mUpdateSubmodules;
  QAction *mInitSubmodules;
  QMenu *mOpenSubmodule;

  // Stash
  QAction *mShowStashes;
  QAction *mStash;
  QAction *mStashPop;

  // History
  QAction *mPrev;
  QAction *mNext;

  // Window
  QAction *mPrevTab;
  QAction *mNextTab;

  static bool sDebugMenuVisible;
};

#endif
