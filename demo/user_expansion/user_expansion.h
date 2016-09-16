// =============================================================================
//  This file is part of:
//  Dynamic Adaptive System for Hierarchical Multipole Methods (DASHMM)
//
//  Copyright (c) 2015-2016, Trustees of Indiana University,
//  All rights reserved.
//
//  DASHMM is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  DASHMM is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with DASHMM. If not, see <http://www.gnu.org/licenses/>.
//
//  This software was created at the Indiana University Center for Research in
//  Extreme Scale Technologies (CREST).
// =============================================================================


// The contents of this file are intended to be a skeleton of what is
// required to define an Expansion type that can be used with DASHMM. The
// comments in these files will explain what is needed and where the user will
// have to implement their specific case.


#ifndef __USER_EXPANSION_H__
#define __USER_EXPANSION_H__


#include <cassert>
#include <cstdio>

#include <memory>

// All of the library interface can be accessed through dashmm.h
#include "dashmm/dashmm.h"


// This is the specific detail of the implementation of the data for the
// User expansion. There are some requirements on the serialization of an
// expansion, and they are represented below. The start of the serialization
// must be two integers. The first is reserved for use by DASHMM. The second
// is the accuracy parameter for this instance of the Expansion. After these,
// the data can take any form so long as it is trivially copyable.
struct UserData {
  int acc;
  //Whatever else that is needed. One example might be
  //
  // int count;
  // double terms[];
  //
  // For a real number expansion with count terms. The form of this
  // extra data will be dictated by the details of the expansion.
  //
  // Another example is that one might require an offset into a table of
  // precomputed values.
  //
  // For now, we settle for the center point.
  dashmm::Point center;
};

// When specializing this to a specific need, there is likely a better name
// than 'User', and so that should be changed to whatever is appropriate.
//
// Expansions are templates over the Source and Target types.
//
// Typical expansions will require that certain members of the Source and
// Target type exist. For instance, many Expansions require that Source and
// Target have a member called 'position' of type dashmm::Point. The specific
// requirements are up to the creator of the Expansion. Only what is used in
// the implementation is required of Source and Target. In this case, this
// thin frame of an Expansion does nothing, so it does not actually place
// any restrictions on Source or Target.
//
// The implementation strategy of Expansion objects is similar to a handle to
// some other memory. The reason for this is that DASHMM will need to send
// serialized versions of the expansion data across the system, and so
// Expansion objects are required to have the capability of releasing their
// data in a contiguous block of memory on request (see release() below).
template <typename Source, typename Target>
class User {
 public:
  // Some convenience aliases (really, only the last is a convenience)
  using source_t = Source;
  using target_t = Target;
  using expansion_t = User<Source, Target>;

  // Inside the library there are a few ways that Expansion objects are
  // constructed. The first is from the expansion point and the accuracy
  // requirement (this is called n_digits here, but it can be whatever number
  // is most convenient for the specific case). This constructor will create
  // a new allocation of memory for the UserData object that this object will
  // handle.
  //
  // DASHMM expects that the data provided by release() will have been allocated
  // using new char [].
  User(dashmm::Point center, double scale, dashmm::ExpansionRole role) {
    // If there was more complication to UserData, this next line would need to
    // be modified.
    bytes_ = sizeof(UserData);
    role_ = role;
    data_ = reinterpret_cast<UserData *>(new char [bytes_]);
    assert(valid(dashmm::ViewSet{}));
    data_->center = center;
  }

  // The second creation method is via capturing some existing UserData and
  // causing this object to handle that memory. As the serialized Expansion
  // data is moved around the system, it will be necessary to interpret that
  // data as the Expansion. This constructor is where this occurs.
  //
  // There are some operations that do not need the actual expansion terms
  // (such as S->T), and so this constructor should also work for
  // ptr == nullptr. The object so constructed will not own any data, but will
  // nevertheless be able to perform those operations not requiring the
  // expansion data. In these 'empty' constructions, n_digits will still be
  // supplied.
  User(const dashmm::ViewSet &views) {
    assert(views.count() < 2);
    bytes_ = sizeof(UserData);
    role_ = views.role();
    if (views.count() == 1) {
      data_ = reinterpret_cast<UserData *>(views.view_data(0));
    } else {
      data_ = nullptr;
    }
  }

  // Expansion objects that still own their data should release that resource
  // when they are destroyed. See release().
  ~User() {
    if (valid(dashmm::ViewSet{})) {
      delete [] data_;
      data_ = nullptr;
    }
  }

  // This routine will supply the serialized version of the data in this
  // expansion object. After a call to release(), this object no longer owns
  // the associated data, and will not free it when this object is destroyed.
  //
  // That this interface is required is the reason that these objects should
  // be implemented as a handle. Otherwise, this method would need to first
  // copy the data into the serialized form before releasing the data. The
  // serialized versions of Expansion objects are interpreted frequently, and
  // are released frequently. So performing these extra copies would impact
  // performance. That being said, one can implement an Expansion in other
  // styles, but one should be aware that there could be performance impacts.
  //
  // Note that not only does release provide the data to the caller, but it will
  // render this object invalid. Otherwise, when this object was destroyed, an
  // attempt would be made to free the data, which is precisely what is not
  // intended.
  void release() {
    data_ = nullptr;
  }

  // This tests if this object is valid; that is, does this object own some
  // data.
  bool valid(const dashmm::ViewSet &view) const {
    assert(view.count() < 2);
    return data_ != nullptr;
  }

  int view_count() const {
    if (data_) return 1;
    return 0;
  }

  void get_views(dashmm::ViewSet &view) const {
    assert(view.count() < 2);
    if (view.count() > 0) {
      view.set_bytes(0, sizeof(UserData));
      view.set_data(0, (char *)data_);
    }
    view.set_role(role_);
  }

  dashmm::ViewSet get_all_views() const {
    dashmm::ViewSet retval{};
    retval.add_view(0);
    get_views(retval);
    return retval;
  }

  dashmm::ExpansionRole role() const {return role_;}

  // This gives the point around which this Expansion is defined.
  dashmm::Point center() const {
    if (valid(dashmm::ViewSet{})) {
      return data_->center;
    } else {
      return dashmm::Point(0.0, 0.0, 0.0);
    }
  }

  // This gives the number of terms in the expansion.
  //
  // Currently, User does not have any terms.
  size_t view_size(int view) const {
    return 0;
  }

  // This returns a specific term in the expansion. dcomplex_t is an
  // alias for std::complex<double>.
  dashmm::dcomplex_t view_term(int view, size_t i) const {
    return dashmm::dcomplex_t{};
  }

  // The following operations (S->M, S->L, M->M, M->L, L->L, M->T, L->T, S->T)
  // would all have to be implemented for the the specific use-case. For now,
  // these all have a simple implementation that spits out a message for each
  // operation.

  // This routine will set this expansion to the multipole moments generated
  // by the given sources.
  std::unique_ptr<User> S_to_M(dashmm::Point center, Source *first,
                               Source *last) const {
    fprintf(stdout, "S->M for %ld sources\n", last - first);
    return std::unique_ptr<User>{new User{center, 1.0,
                                          dashmm::kSourcePrimary}};
  }

  // This will generate a local expansion at the given center for the
  // given points.
  std::unique_ptr<User> S_to_L(dashmm::Point center, Source *first,
                               Source *last) const {
    fprintf(stdout, "S->L for %ld sources\n", last - first);
    return std::unique_ptr<User>{new User{center, 1.0,
                                          dashmm::kTargetPrimary}};
  }

  // This will translate a multipole expansion from a child node to its parent.
  std::unique_ptr<User> M_to_M(int from_child, double s_size) const {
    fprintf(stdout, "M->M from child %d\n", from_child);
    double h = s_size / 2;
    double px = data_->center.x() + (from_child % 2 == 0 ? h : -h);
    double py = data_->center.y() + (from_child % 4 <= 1 ? h : -h);
    double pz = data_->center.z() + (from_child < 4 ? h : -h);
    return std::unique_ptr<User>{new User{dashmm::Point{px, py, pz},
                                          1.0, dashmm::kSourcePrimary}};
  }

  // This will translate a multipole expansion into a local expansion.
  std::unique_ptr<User> M_to_L(dashmm::Index s_index, double s_size,
                               dashmm::Index t_index) const {
    fprintf(stdout, "M->L\n");
    int t2s_x = s_index.x() - t_index.x();
    int t2s_y = s_index.y() - t_index.y();
    int t2s_z = s_index.z() - t_index.z();
    double tx = data_->center.x() - t2s_x * s_size;
    double ty = data_->center.y() - t2s_y * s_size;
    double tz = data_->center.z() - t2s_z * s_size;
    return std::unique_ptr<User>{new User{dashmm::Point{tx, ty, tz},
                                          1.0, dashmm::kTargetPrimary}};
  }

  // This will translate a local expansion from a parent node to one of
  // its children.
  std::unique_ptr<User> L_to_L(int to_child, double t_size) const {
    fprintf(stdout, "L->L to child %d\n", to_child);
    double h = t_size / 2;
    double cx = data_->center.x() + (to_child % 2 == 0 ? -h : h);
    double cy = data_->center.y() + (to_child % 4 <= 1 ? -h : h);
    double cz = data_->center.z() + (to_child < 4 ? -h : h);
    return std::unique_ptr<User>{new User{dashmm::Point{cx, cy, cz},
                                          1.0, dashmm::kTargetPrimary}};
  }

  // This will compute the effect of a multipole expansion on a set of
  // target points.
  void M_to_T(target_t *first, target_t *last) const {
    fprintf(stdout, "M->T for %ld targets\n", last - first);
  }

  // This will compute the effect of a local expansion on a set of target
  // points.
  void L_to_T(target_t *first, target_t *last) const {
    fprintf(stdout, "L->T for %ld targets\n", last - first);
  }

  // This will compute the effect of a set of sources on a set of target points.
  void S_to_T(source_t *s_first, source_t *s_last,
              target_t *t_first, target_t *t_last) const {
    fprintf(stdout, "S->T for %ld sources and %ld targets\n",
            s_last - s_first, t_last - t_first);
  }

  std::unique_ptr<expansion_t> M_to_I(dashmm::Index s_index) const {
    return std::unique_ptr<expansion_t>{nullptr};
  }

  std::unique_ptr<expansion_t> I_to_I(dashmm::Index s_index, double s_size,
                                      dashmm::Index t_index) const {
    return std::unique_ptr<expansion_t>{nullptr};
  }

  std::unique_ptr<expansion_t> I_to_L(dashmm::Index t_index,
                                      double t_size) const {
    return std::unique_ptr<expansion_t>{nullptr};
  }

  // The routine will add the provided expansion to this expansion.
  void add_expansion(const User *temp1) {
    fprintf(stdout, "Adding an expansion\n");
  }

  static void update_table(int n_digits, double domain_size,
                           const std::vector<double> &kernel_params) { }

  static void delete_table() { }

  static double compute_scale(dashmm::Index index) {return 1.0;}

  static int weight_estimate(dashmm::Operation op,
                             dashmm::Index s = dashmm::Index{},
                             dashmm::Index t = dashmm::Index{}) {
    return 1;
  }

 private:
  // This object acts as a handle to some other piece of memory. The details
  // of the UserData type can be found in user_expansion.cc.
  UserData *data_;

  // This stores the size (in bytes) of data_.
  size_t bytes_;

  dashmm::ExpansionRole role_;
};


#endif // __USER_EXPANSION_H__
